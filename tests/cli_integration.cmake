file(MAKE_DIRECTORY "${OUTPUT_DIR}")
file(SHA256 "${INPUT}" input_hash)

function(run_effect effect suffix)
    set(output "${OUTPUT_DIR}/${suffix}.ppm")
    execute_process(
        COMMAND "${PRISMFORGE}" --input "${INPUT}" --output "${output}" --effect "${effect}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "CLI effect ${effect} failed (${result}): ${stderr}")
    endif()
    if(NOT EXISTS "${output}")
        message(FATAL_ERROR "CLI effect ${effect} did not write ${output}")
    endif()
    file(READ "${output}" magic_hex LIMIT 2 HEX)
    if(NOT magic_hex STREQUAL "5036")
        message(FATAL_ERROR "CLI effect ${effect} did not write a P6 PPM")
    endif()
    file(SHA256 "${output}" output_hash)
    if(output_hash STREQUAL input_hash)
        message(FATAL_ERROR "CLI effect ${effect} unexpectedly copied the input unchanged")
    endif()
endfunction()

run_effect(blur blur --radius 3 --sigma 1.5)
run_effect(sobel sobel)
run_effect(color color --brightness 12 --contrast 1.1 --saturation 1.25)

execute_process(
    COMMAND "${PRISMFORGE}" --input "${INPUT}" --output "${OUTPUT_DIR}/invalid.ppm" --effect unknown
    RESULT_VARIABLE invalid_effect_result)
if(NOT invalid_effect_result EQUAL 2)
    message(FATAL_ERROR "CLI unknown effect returned ${invalid_effect_result}, expected 2")
endif()

execute_process(
    COMMAND "${PRISMFORGE}" --input "${INPUT}" --output "${OUTPUT_DIR}/invalid-option.ppm" --effect sobel --unknown value
    RESULT_VARIABLE invalid_option_result)
if(NOT invalid_option_result EQUAL 2)
    message(FATAL_ERROR "CLI unknown option returned ${invalid_option_result}, expected 2")
endif()

execute_process(
    COMMAND "${PRISMFORGE}" --input "${OUTPUT_DIR}/missing.ppm" --output "${OUTPUT_DIR}/missing-output.ppm" --effect sobel
    RESULT_VARIABLE missing_input_result)
if(NOT missing_input_result EQUAL 2)
    message(FATAL_ERROR "CLI missing input returned ${missing_input_result}, expected 2")
endif()

message(STATUS "CLI integration verified all three filters and invalid input/option handling")
