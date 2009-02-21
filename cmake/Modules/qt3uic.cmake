

# used internally by KDE3Macros.cmake
# neundorf@kde.org


EXECUTE_PROCESS(COMMAND ${KDE_UIC_EXECUTABLE}
   -L ${KDE_UIC_PLUGIN_DIR} -nounload 
   -impl ${KDE_UIC_H_FILE}
   ${KDE_UIC_FILE}
   OUTPUT_VARIABLE _uic_CONTENTS
   ERROR_QUIET
  )

STRING(REGEX REPLACE "tr2i18n\\(\"\"\\)" "QString::null" _uic_CONTENTS "${_uic_CONTENTS}" )
STRING(REGEX REPLACE "tr2i18n\\(\"\", \"\"\\)" "QString::null" _uic_CONTENTS "${_uic_CONTENTS}" )

FILE(WRITE ${KDE_UIC_CPP_FILE} "\n\n")
FILE(APPEND ${KDE_UIC_CPP_FILE} "${_uic_CONTENTS}")

