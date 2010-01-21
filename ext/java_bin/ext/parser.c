#include "parser.h"

/*
 * variables
 */
static VALUE rb_mJavaBin;
static VALUE rb_mExt;
static VALUE rb_cParser;

static ID    i_At; // Time#at用
#ifdef HAVE_RUBY_ENCODING_H
static rb_encoding* rb_encUtf8;
#endif

/*
 * javabinフォーマット読み込み関数群
 */

static int32_t JavaBinParser_read_v_int(JAVA_BIN_PARSER* ptr) {
  unsigned char byte;
  int32_t result;
  int shift;
  byte = _getbyte(ptr);
  result = byte & 0x7f;
  for(shift = 7; (byte & 0x80) != 0; shift += 7) {
    byte = _getbyte(ptr);
    result |= (((int32_t)(byte & 0x7f)) << shift);
  }
  return result;
}

static int64_t JavaBinParser_read_v_long(JAVA_BIN_PARSER* ptr) {
  unsigned char byte;
  int64_t result;
  int shift;
  byte = _getbyte(ptr);
  result = byte & 0x7f;
  for(shift = 7; (byte & 0x80) != 0; shift += 7) {
    byte = _getbyte(ptr);
    result |= (((int64_t)(byte & 0x7f)) << shift);
  }
  return result;
}

static int JavaBinParser_read_size(JAVA_BIN_PARSER* ptr) {
  int size;
  size = (ptr->tag_byte & 0x1f);
  if (size == 0x1f) {
    size += JavaBinParser_read_v_int(ptr);
  }
  return size;
}

static VALUE JavaBinParser_read_small_int(JAVA_BIN_PARSER* ptr) {
  int32_t result;
  result = ptr->tag_byte & 0x0f;
  if ((ptr->tag_byte & 0x10) != 0) {
    result = ((JavaBinParser_read_v_int(ptr) << 4) | result);
  }
  return INT2NUM(result);
}

static VALUE JavaBinParser_read_small_long(JAVA_BIN_PARSER* ptr) {
  int64_t result;
  result = ptr->tag_byte & 0x0f;
  if ((ptr->tag_byte & 0x10) != 0) {
    result = ((JavaBinParser_read_v_long(ptr) << 4) | result);
  }
  return LL2NUM(result);
}

static VALUE JavaBinParser_read_string(JAVA_BIN_PARSER* ptr) {
  int size;
  int i;
  int start;
  unsigned char b;

  size = JavaBinParser_read_size(ptr);
  start = ptr->current;
  for (i = 0; i < size; i++) {
    /* HINT. read utf-8 char */
    b = _getbyte(ptr);
    if ((b & 0x80) == 0) {
    } else if ((b & 0xE0) == 0xC0) {
      _skipbytes(ptr, 1);
    } else {
      _skipbytes(ptr, 2);
    } /* TODO 4byte以上のケース? */
  }
  return _utf8_string((const char*) &ptr->data[start], ptr->current - start); 
}

static VALUE JavaBinParser_read_byte(JAVA_BIN_PARSER* ptr) {
  int8_t c;
  _readnumeric(ptr, c);
  return INT2FIX(*((int8_t*)&c));
}

static VALUE JavaBinParser_read_short(JAVA_BIN_PARSER* ptr) {
  u_int16_t c;
  _readnumeric(ptr, c);
  c = _swap_16(c);
  return INT2FIX(*((int16_t*)&c));
}

static VALUE JavaBinParser_read_int(JAVA_BIN_PARSER* ptr) {
  u_int32_t c;
  _readnumeric(ptr, c);
  c = _swap_32(c);
  return INT2NUM(*((int32_t*)&c));
}

static VALUE JavaBinParser_read_long(JAVA_BIN_PARSER* ptr) {
  u_int64_t c;
  _readnumeric(ptr, c);
  c = _swap_64(c);
  return LL2NUM(*((int64_t*)&c));
}

static VALUE JavaBinParser_read_date(JAVA_BIN_PARSER* ptr) {
  u_int64_t c;
  _readnumeric(ptr, c);
  c = _swap_64(c);
  return rb_funcall(rb_cTime, i_At, 1, ULL2NUM(*((int64_t*)&c) / 1000));
}

static VALUE JavaBinParser_read_float(JAVA_BIN_PARSER* ptr) {
  u_int32_t c;
  _readnumeric(ptr, c);
  c = _swap_32(c);
  return rb_float_new((double)*((float*)&c));
}

static VALUE JavaBinParser_read_double(JAVA_BIN_PARSER* ptr) {
  u_int64_t c;
  _readnumeric(ptr, c);
  c = _swap_64(c);
  return rb_float_new(*((double*)&c));
}

static void JavaBinParser_extend_cache(JAVA_BIN_PARSER* ptr) {
  VALUE* newP;
  int next_size;
  if (ptr->cache == NULL) {
    next_size = 64;
  } else {
    next_size = ptr->cache_size * 2;
  }

  newP = (VALUE*) malloc(next_size * sizeof(VALUE));
  if (!newP) {
    rb_raise(rb_eRuntimeError, "JavaBinParser_extend_cache - allocate error");
  }

  if (ptr->cache) {
    memcpy(newP, ptr->cache, sizeof(VALUE) * ptr->cache_size);
  }
  ptr->cache = newP;
  ptr->cache_size = next_size;
}

static VALUE JavaBinParser_read_val(JAVA_BIN_PARSER* ptr) {
  int size;
  int i;
  VALUE key;
  VALUE value;
  VALUE array;
  VALUE hash;

  ptr->tag_byte = _getbyte(ptr);
  switch (ptr->tag_byte >> 5) { /* unsignedなので論理シフト */
    case SHIFTED_STR:
      return JavaBinParser_read_string(ptr);
    case SHIFTED_ARR:
      size = JavaBinParser_read_size(ptr);
      array = rb_ary_new();
      for (i = 0; i < size; i++) {
        value = JavaBinParser_read_val(ptr);
        rb_ary_push(array, value);
      }
      return array;
    case SHIFTED_EXTERN_STRING:
      size = JavaBinParser_read_size(ptr);
      if(size == 0) {
        /* rubyの文字列 */
        value = JavaBinParser_read_val(ptr);

        /* 参照文字列として文字列を保持 */
        ptr->cache[ptr->cache_index++] = value;
        /* 参照文字列用のcacheを拡張する */
        if (ptr->cache_size <= ptr->cache_index) {
          JavaBinParser_extend_cache(ptr);
        }
        return value;
      } else {
        return rb_str_new4(ptr->cache[size - 1]);   // freeze共有
        //return rb_str_new3(ptr->cache[size - 1]); // 共有(変更があったら分裂)
        
	//return ptr->cache[size - 1];              // 同じ物
        //return rb_str_dup(ptr->cache[size - 1]);  // コピー
      }
    case SHIFTED_ORDERED_MAP:
    case SHIFTED_NAMED_LST:
      size = JavaBinParser_read_size(ptr);
      hash = rb_hash_new();
      for (i = 0; i < size; i++) {
        key   = JavaBinParser_read_val(ptr);
        value = JavaBinParser_read_val(ptr);
        rb_hash_aset(hash, key, value);
      }
      return hash;
    case SHIFTED_SINT:
      return JavaBinParser_read_small_int(ptr);
    case SHIFTED_SLONG:
      return JavaBinParser_read_small_long(ptr);
  }

  switch(ptr->tag_byte) {
    case BYTE:
      return JavaBinParser_read_byte(ptr);
    case SHORT:
      return JavaBinParser_read_short(ptr);
    case DOUBLE:
      return JavaBinParser_read_double(ptr);
    case INT:
      return JavaBinParser_read_int(ptr);
    case LONG:
      return JavaBinParser_read_long(ptr);
    case FLOAT:
      return JavaBinParser_read_float(ptr);
    case DATE:
      return JavaBinParser_read_date(ptr);
    case BYTEARR:
      size = JavaBinParser_read_v_int(ptr);
      array = rb_ary_new();
      for (i = 0; i < size; i++) {
        rb_ary_push(array, INT2FIX(_getbyte(ptr)));
      }
      return array;
    case NULL_MARK:
      return Qnil;
    case BOOL_TRUE:
      return Qtrue;
    case BOOL_FALSE:
      return Qfalse;
    case MAP:
      size = JavaBinParser_read_v_int(ptr);
      hash = rb_hash_new();
      for (i = 0; i < size; i++) {
        key   = JavaBinParser_read_val(ptr);
        value = JavaBinParser_read_val(ptr);
        rb_hash_aset(hash, key, value);
      }
      return hash;
   case ITERATOR:
      array = rb_ary_new();
      while (1) {
        value = JavaBinParser_read_val(ptr);
        if (value == END_OBJ) {
          break;
        }
        rb_ary_push(array, value);
      }
      return array;
    case END:
      return END_OBJ; 
    case SOLRDOC:
      return JavaBinParser_read_val(ptr);
    case SOLRDOCLST:
      hash = rb_hash_new();
      value = JavaBinParser_read_val(ptr);
      // TODO キーのfreeze
      rb_hash_aset(hash, rb_str_new2("numFound"), rb_ary_entry(value, 0));
      rb_hash_aset(hash, rb_str_new2("start"),    rb_ary_entry(value, 1));
      rb_hash_aset(hash, rb_str_new2("maxScore"), rb_ary_entry(value, 2));
      rb_hash_aset(hash, rb_str_new2("docs"),     JavaBinParser_read_val(ptr));
      return hash;
    default:
      rb_raise(rb_eRuntimeError, "JavaBinParser_read_val - unknown tag type");
  }
}

static void JavaBinParser_free(JAVA_BIN_PARSER* ptr) {
  if (ptr) {
    //free(ptr->data);
    if (ptr->cache) {
      free(ptr->cache);
    }
    free(ptr);
  }
}

static void JavaBinParser_mark(JAVA_BIN_PARSER* ptr) {
  int i;
  if (ptr) {
    for (i = 0; i < ptr->cache_index; i++) {
      rb_gc_mark_maybe(ptr->cache[i]);
    }
  }
}

static VALUE JavaBinParser_alloc(VALUE klass) {
  return Data_Wrap_Struct(klass, JavaBinParser_mark, JavaBinParser_free, NULL);
}

/*
 * rubyメソッド
 */
static VALUE rb_cParser_parse(VALUE self, VALUE data) {
  JAVA_BIN_PARSER* ptr;
  char* ptrData;
  int   dataLen;

  Data_Get_Struct(self, JAVA_BIN_PARSER, ptr);

  /* 引数処理 */
  if (TYPE(data) != T_STRING) {
    rb_raise(rb_eRuntimeError, "rb_cParser_parse - data is not String.");
  }

  SafeStringValue(data);
  ptrData = RSTRING_PTR(data);
  dataLen = RSTRING_LEN(data);

  /* 引数チェック */
  if (ptrData == NULL || dataLen == 0) {
    rb_raise(rb_eRuntimeError, "rb_cParser_parse - data is empty.");
  }

  //ptr->data = (unsigned char*) malloc(dataLen);
  //if (!ptr->data) {
  //  rb_raise(rb_eRuntimeError, "rb_cParser_parse - allocate error");
  //}
  //memcpy(ptr->data, ptrData, dataLen);
  ptr->data = (unsigned char*)ptrData;
  ptr->data_len = dataLen;

  /* version check */
  if (ptr->data[0] != 0x01) {
    rb_raise(rb_eRuntimeError, "rb_cParser_parse - not supported version");
  }

  ptr->current  = 1;   /* VERSIONをとばした */
  ptr->tag_byte = 0;

  /*
   * 参照文字列既に確保している場合は解放
   * HINT. インスタンスを使いまわす時に発生する
   */
  if (ptr->cache) {
    free(ptr->cache);
  }

  /* 参照文字列の準備 */
  ptr->cache = NULL;
  ptr->cache_index = 0;
  JavaBinParser_extend_cache(ptr);
 
  return JavaBinParser_read_val(ptr);
}

static VALUE rb_cParser_initialize(VALUE self) {
  JAVA_BIN_PARSER* ptr;

  /* データの初期化 */
  ptr = (JAVA_BIN_PARSER*) malloc(sizeof(JAVA_BIN_PARSER));
  if (!ptr) {
    rb_raise(rb_eRuntimeError, "rb_cParser_initialize - allocate error");
  }
  DATA_PTR(self) = ptr;

  /* 参照文字列の準備(ここでも初期化しておかないと、たまにsegvしちゃいますruby 1.8.7) */
  ptr->cache = NULL;
  ptr->cache_index = 0;
 
  return self;
}

/*
 * エントリーポイント
 */
void Init_parser(void) {
  i_At = rb_intern("at");
#ifdef HAVE_RUBY_ENCODING_H
  rb_encUtf8 = rb_utf8_encoding();
#endif

  /* クラス定義 */
  rb_mJavaBin = rb_define_module("JavaBin");
  rb_mExt     = rb_define_module_under(rb_mJavaBin, "Ext");
  rb_cParser  = rb_define_class_under(rb_mExt, "Parser", rb_cObject);
  
  /* メモリーアロケーター設定 */
  rb_define_alloc_func(rb_cParser, JavaBinParser_alloc);
  /* コンストラクタ */
  rb_define_method(rb_cParser, "initialize", rb_cParser_initialize, 0);
  /* parseメソッド*/
  rb_define_method(rb_cParser, "parse", rb_cParser_parse, 1);
}

