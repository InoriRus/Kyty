execute_process(COMMAND git describe --tags --always OUTPUT_VARIABLE KYTY_GIT_VERSION)
string(STRIP ${KYTY_GIT_VERSION} KYTY_GIT_VERSION)
configure_file(${INPUT_FILE} ${OUTPUT_FILE})
