


add_requires("doctest")
add_rules("mode.debug")
add_rules("mode.release")

target("test")
  set_kind("binary")
--  add_files("src/*.cpp")
  add_files("test/*.cpp")
  set_languages("cxx17")
  add_includedirs("include")
  set_warnings("allextra")
  
  add_packages("doctest")
