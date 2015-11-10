# Build hal/utils/comp

# In yapps.py, 'import yapps' tries to import the script itself;
# rename it
add_custom_command(OUTPUT yapps
  COMMAND install -m 755 ${CMAKE_CURRENT_SOURCE_DIR}/yapps.py yapps
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/yapps.py
  DEPENDS ${PYTHONPATH}/yapps/*.py
  COMMENT "Copying yapps python executable"
  )

add_custom_command(OUTPUT comp
  COMMAND ${LOCAL_PYTHON_EXECUTABLE} yapps
  ARGS        ${CMAKE_CURRENT_SOURCE_DIR}/comp.g comp
  COMMAND chmod +x comp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/comp.g yapps
  COMMENT "Generating comp python executable"
  )

add_custom_target(comp_executable ALL DEPENDS comp)

