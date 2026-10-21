file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

function(run_cli expected label)
    execute_process(
        COMMAND "${PRISMFORGE}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    if(NOT result EQUAL expected)
        message(FATAL_ERROR "CLI ${label} returned ${result}, expected ${expected}: ${stderr} ${stdout}")
    endif()
endfunction()

function(assert_magic path expected_hex)
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "expected CLI output is missing: ${path}")
    endif()
    file(READ "${path}" actual_hex LIMIT 8 HEX)
    if(NOT actual_hex MATCHES "^${expected_hex}")
        message(FATAL_ERROR "unexpected magic bytes in ${path}: ${actual_hex}")
    endif()
endfunction()

# Produce PNG and JPEG inputs with the executable, then test all 3x3 input/output
# format combinations by reopening every result through the actual CLI decoder.
set(input_png "${OUTPUT_DIR}/seed.png")
set(input_jpg "${OUTPUT_DIR}/seed.jpg")
run_cli(0 "PPM to PNG" --input "${INPUT}" --output "${input_png}" --filter invert)
run_cli(0 "PPM to JPEG" --input "${INPUT}" --output "${input_jpg}" --quality 91 --filter grayscale)
assert_magic("${input_png}" "89504e470d0a1a0a")
assert_magic("${input_jpg}" "ffd8ff")

set(input_paths "${INPUT}" "${input_png}" "${input_jpg}")
set(input_labels ppm png jpg)
set(output_extensions ppm png jpg)
set(output_magic 5036 89504e470d0a1a0a ffd8ff)
list(LENGTH input_paths input_count)
math(EXPR input_last "${input_count} - 1")
foreach(input_index RANGE ${input_last})
    list(GET input_paths ${input_index} source_path)
    list(GET input_labels ${input_index} source_label)
    foreach(output_index RANGE 0 2)
        list(GET output_extensions ${output_index} extension)
        list(GET output_magic ${output_index} signature)
        set(output_path "${OUTPUT_DIR}/${source_label}-to-${extension}.${extension}")
        run_cli(0 "${source_label} to ${extension}" --input "${source_path}" --output "${output_path}" --filter "color:brightness=5")
        assert_magic("${output_path}" "${signature}")
        set(reopened "${OUTPUT_DIR}/${source_label}-to-${extension}-reopened.ppm")
        run_cli(0 "reopen ${source_label} to ${extension}" --input "${output_path}" --output "${reopened}" --overwrite)
        assert_magic("${reopened}" "5036")
    endforeach()
endforeach()

set(pipeline_output "${OUTPUT_DIR}/pipeline.png")
run_cli(0 "ordered multi-filter pipeline" --input "${INPUT}" --output "${pipeline_output}"
    --filter "blur:radius=2,sigma=1.1" --filter "sharpen:amount=0.6"
    --filter "color:brightness=8,saturation=1.1")
assert_magic("${pipeline_output}" "89504e470d0a1a0a")

run_cli(0 "legacy single-effect syntax" --input "${INPUT}" --output "${OUTPUT_DIR}/legacy.ppm"
    --effect blur --radius 2 --sigma 1.0)
run_cli(2 "unknown filter" --input "${INPUT}" --output "${OUTPUT_DIR}/bad.ppm" --filter "imaginary")
run_cli(2 "malformed filter parameter" --input "${INPUT}" --output "${OUTPUT_DIR}/bad-param.ppm" --filter "blur:radius=-3")
run_cli(2 "invalid JPEG quality" --input "${INPUT}" --output "${OUTPUT_DIR}/bad-quality.jpg" --quality 101)
run_cli(2 "missing input" --input "${OUTPUT_DIR}/missing.ppm" --output "${OUTPUT_DIR}/missing-out.png")
run_cli(2 "unknown option" --input "${INPUT}" --output "${OUTPUT_DIR}/bad-option.ppm" --unknown value)
run_cli(0 "version" --version)

set(batch_input "${OUTPUT_DIR}/batch-input")
set(batch_output "${OUTPUT_DIR}/batch-output")
file(MAKE_DIRECTORY "${batch_input}")
configure_file("${INPUT}" "${batch_input}/sample.ppm" COPYONLY)
configure_file("${input_png}" "${batch_input}/sample-png.png" COPYONLY)
configure_file("${input_jpg}" "${batch_input}/sample-jpg.jpg" COPYONLY)
file(WRITE "${batch_input}/notes.txt" "predictably skipped\n")
run_cli(0 "batch mode" --input-dir "${batch_input}" --output-dir "${batch_output}"
    --filter grayscale --format png)
foreach(name sample.ppm sample-png.png sample-jpg.jpg)
    get_filename_component(stem "${name}" NAME_WE)
    if(NOT EXISTS "${batch_output}/${stem}.png")
        message(FATAL_ERROR "batch output missing for ${name}")
    endif()
endforeach()
run_cli(2 "overlapping batch directories" --input-dir "${batch_input}" --output-dir "${batch_input}/nested")
run_cli(1 "batch output collision" --input-dir "${batch_input}" --output-dir "${batch_output}" --format png)
run_cli(0 "batch explicit overwrite" --input-dir "${batch_input}" --output-dir "${batch_output}" --format png --overwrite)

message(STATUS "CLI verified all format conversions, pipelines, batch behavior, errors, and version output")
