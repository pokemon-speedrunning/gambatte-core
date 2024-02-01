function(get_available_compile_flags OUT_VAR)
  if(MSVC)
    set(FLAGS_TO_CHECK "/W4" "/GR-")
    set(WARNINGS_AS_ERROR "/WX")
  else()
    set(FLAGS_TO_CHECK
      "-Wall"
      "-Wextra"
      "-fno-rtti"
      "-fno-exceptions"
      "-fvisibility=hidden"
      "-fvisibility-inlines-hidden"
    )
    set(CXX_ONLY_FLAGS "-fno-rtti" "-fno-exceptions" "-fvisibility-inlines-hidden")
    set(WARNINGS_AS_ERROR "-Werror")
  endif()

  include(CheckCXXCompilerFlag)

  foreach(FLAG IN LISTS FLAGS_TO_CHECK)
    string(TOUPPER "${FLAG}" UPPER_FLAG)
    string(MAKE_C_IDENTIFIER "${UPPER_FLAG}" FLAG_NAME)
    check_cxx_compiler_flag("${FLAG}" "HAVE${FLAG_NAME}")

    if(HAVE${FLAG_NAME})
      if(FLAG IN_LIST CXX_ONLY_FLAGS)
        list(APPEND SUPPORTED_FLAGS $<$<COMPILE_LANGUAGE:CXX>:${FLAG}>)
      else()
        list(APPEND SUPPORTED_FLAGS "${FLAG}")
      endif()
    endif()
  endforeach()

  check_cxx_compiler_flag("${WARNINGS_AS_ERROR}" HAVE_WARNINGS_AS_ERROR)
  include(CMakeDependentOption)

  # Hide the option altogether if this is included as a subproject
  cmake_dependent_option(LIBGAMBATTE_WARNINGS_AS_ERROR "Treat all warnings as error" OFF "HAVE_WARNINGS_AS_ERROR;NOT LIBGAMBATTE_IS_SUBPROJECT" OFF)

  if(LIBGAMBATTE_WARNINGS_AS_ERROR)
    list(APPEND SUPPORTED_FLAGS "${WARNINGS_AS_ERROR}")
  endif()

  set(${OUT_VAR} ${SUPPORTED_FLAGS} PARENT_SCOPE)
endfunction()
