# Generated by jeweler
# DO NOT EDIT THIS FILE DIRECTLY
# Instead, edit Jeweler::Tasks in Rakefile, and run the gemspec command
# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{java_bin}
  s.version = "0.3.0"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["kennyj"]
  s.date = %q{2010-01-22}
  s.description = %q{Apache Solr JavaBin format (binary format) implementation for Ruby.}
  s.email = %q{kennyj@gmail.com}
  s.extensions = ["ext/java_bin/ext/extconf.rb"]
  s.extra_rdoc_files = [
    "LICENSE",
     "README.rdoc"
  ]
  s.files = [
    ".document",
     ".gitignore",
     "CHANGELOG",
     "LICENSE",
     "README.rdoc",
     "Rakefile",
     "VERSION",
     "ext/java_bin/ext/extconf.rb",
     "ext/java_bin/ext/parser.c",
     "ext/java_bin/ext/parser.h",
     "fixtures/javabin.dat",
     "fixtures/javabin2.dat",
     "fixtures/json.dat",
     "fixtures/json2.dat",
     "fixtures/ruby.dat",
     "fixtures/ruby2.dat",
     "java_bin.gemspec",
     "lib/java_bin.rb",
     "lib/java_bin/ext.rb",
     "lib/java_bin/ext/.keep",
     "lib/java_bin/pure.rb",
     "lib/java_bin/pure/parser.rb",
     "lib/java_bin/version.rb",
     "lib/rsolr_support.rb",
     "lib/solr_ruby_support.rb",
     "test/helper.rb",
     "test/test_java_bin_parser.rb",
     "test/xxx_performance.rb"
  ]
  s.homepage = %q{http://github.com/kennyj/java_bin}
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["lib", "ext"]
  s.rubygems_version = %q{1.3.5}
  s.summary = %q{Apache Solr JavaBin format implementation for Ruby.}
  s.test_files = [
    "test/test_java_bin_parser.rb",
     "test/xxx_performance.rb",
     "test/helper.rb"
  ]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 3

    if Gem::Version.new(Gem::RubyGemsVersion) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end

