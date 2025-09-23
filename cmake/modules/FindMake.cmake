function(find_make make_exe make_cmd)
  # make_exe the name of the variable whose value will be the path to "make"
  #          executable
  # make_cmd the name of the variable whose value will be the command to
  #          used in the generated build script executed by the cmake generator
  find_program(MAKE_EXECUTABLE NAMES gmake make)
  if(NOT MAKE_EXECUTABLE)
    message(FATAL_ERROR "Can't find make")
  endif()
  set(${make_exe} "${MAKE_EXECUTABLE}" PARENT_SCOPE)
  set(${make_cmd} "${MAKE_EXECUTABLE}" PARENT_SCOPE)
endfunction()
