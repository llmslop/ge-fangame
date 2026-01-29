find_package(Python3 REQUIRED)

add_library(ge-assets STATIC)
target_include_directories(ge-assets PUBLIC ${CMAKE_BINARY_DIR}/generated)

function(bin2c_generic BIN2C SYMBOL_NAME RESOURCE_FILE)
    cmake_parse_arguments(
        BIN2C # prefix
        "" # no boolean options
        "OUTPUT_BASE" # single-value keywords
        "ARGS" # multi-value keywords
        ${ARGN}
    )

    # ---- OUTPUT_BASE default ----
    if(BIN2C_OUTPUT_BASE)
        set(OUTPUT_BASE "${BIN2C_OUTPUT_BASE}")
    else()
        get_filename_component(_dir "${RESOURCE_FILE}" DIRECTORY)
        get_filename_component(_name "${RESOURCE_FILE}" NAME_WE)
        set(OUTPUT_BASE "${_dir}/${_name}")
    endif()

    set(ADDITIONAL_ARGS ${BIN2C_ARGS})

    # ---- Paths ----
    set(SCRIPT_FILE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/scripts/${BIN2C})

    set(HEADER_FILE ${CMAKE_BINARY_DIR}/generated/assets/${OUTPUT_BASE}.h)

    set(SOURCE_FILE ${CMAKE_BINARY_DIR}/generated/assets/${OUTPUT_BASE}.c)

    get_filename_component(
        RESOURCE_FILE_ABSOLUTE
        "${RESOURCE_FILE}"
        ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}"
    )

    message(STATUS "${BIN2C}: ${RESOURCE_FILE} → ${SOURCE_FILE}")

    add_custom_command(
        OUTPUT ${HEADER_FILE} ${SOURCE_FILE}
        COMMAND
            ${Python3_EXECUTABLE} ${SCRIPT_FILE} ${RESOURCE_FILE_ABSOLUTE} ${SOURCE_FILE} ${HEADER_FILE} ${SYMBOL_NAME}
            ${ADDITIONAL_ARGS}
        DEPENDS ${RESOURCE_FILE} ${SCRIPT_FILE} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/scripts/bin2c.py
        VERBATIM
    )

    target_sources(ge-assets PRIVATE ${SOURCE_FILE} ${HEADER_FILE})
endfunction()

function(raw_audio SYMBOL_NAME WAV_FILE)
    cmake_parse_arguments(
        RAW_AUDIO # prefix
        "" # no boolean options
        "" # no single-value keywords
        "ARGS" # multi-value keywords
        ${ARGN}
    )

    bin2c_generic(bin2c_audio.py ${SYMBOL_NAME} ${WAV_FILE} ARGS ${RAW_AUDIO_ARGS})
endfunction()

function(raw_image SYMBOL_NAME IMAGE_FILE)
    cmake_parse_arguments(
        RAW_IMAGE # prefix
        "" # no boolean options
        "" # no single-value keywords
        "ARGS" # multi-value keywords
        ${ARGN}
    )

    # Default mode is rgb565, but allow override via ARGS
    if(NOT RAW_IMAGE_ARGS)
        set(RAW_IMAGE_ARGS rgb565)
    endif()

    bin2c_generic(bin2c_image.py ${SYMBOL_NAME} ${IMAGE_FILE} ARGS ${RAW_IMAGE_ARGS})
endfunction()

function(raw_image_alpha SYMBOL_NAME IMAGE_FILE)
    cmake_parse_arguments(
        RAW_IMAGE_ALPHA # prefix
        "" # no boolean options
        "" # no single-value keywords
        "ARGS" # multi-value keywords
        ${ARGN}
    )

    # Default mode is argb1555, but allow override via ARGS
    if(NOT RAW_IMAGE_ALPHA_ARGS)
        set(RAW_IMAGE_ALPHA_ARGS argb8888)
    endif()

    bin2c_generic(bin2c_image.py ${SYMBOL_NAME} ${IMAGE_FILE} ARGS ${RAW_IMAGE_ALPHA_ARGS})
endfunction()

function(bitmap_font SYMBOL_NAME FONT_FILE FONT_SIZE)
    cmake_parse_arguments(
        BITMAP_FONT # prefix
        "" # no boolean options
        "" # no single-value keywords
        "ARGS" # multi-value keywords
        ${ARGN}
    )

    get_filename_component(_dir ${FONT_FILE} DIRECTORY)
    get_filename_component(_name ${FONT_FILE} NAME_WE)
    bin2c_generic(
        bin2c_bitmap_font.py
        ${SYMBOL_NAME}
        ${FONT_FILE}
        OUTPUT_BASE
        "${_dir}/${_name}_${FONT_SIZE}px"
        ARGS
        ${FONT_SIZE}
    )
endfunction()

function(raw_image_rotated SYMBOL_NAME IMAGE_FILE ROTATION_ANGLE)
    cmake_parse_arguments(
        RAW_IMAGE_ROT # prefix
        "" # no boolean options
        "MODE" # single-value keywords
        "ARGS" # multi-value keywords
        ${ARGN}
    )

    # Default mode is argb1555 for rotated images
    # (rotated images often need alpha channel for smooth edges due to interpolation)
    if(NOT RAW_IMAGE_ROT_MODE)
        set(RAW_IMAGE_ROT_MODE argb1555)
    endif()

    bin2c_generic(
        bin2c_image.py
        ${SYMBOL_NAME}
        ${IMAGE_FILE}
        ARGS
        ${RAW_IMAGE_ROT_MODE}
        --rotate
        ${ROTATION_ANGLE}
        ${RAW_IMAGE_ROT_ARGS}
    )
endfunction()

function(raw_image_animated SYMBOL_NAME IMAGE_FILE)
    cmake_parse_arguments(
        RAW_IMAGE_ANIM # prefix
        "" # no boolean options
        "MODE" # single-value keywords
        "ARGS" # multi-value keywords
        ${ARGN}
    )

    # # Default mode is argb1555 for animated images
    # # (animated images often have transparency for sprite composition)
    # if(NOT RAW_IMAGE_ANIM_MODE)
    #     set(RAW_IMAGE_ANIM_MODE argb1555)
    # endif()

    bin2c_generic(
        bin2c_image.py
        ${SYMBOL_NAME}
        ${IMAGE_FILE}
        ARGS
        ${RAW_IMAGE_ANIM_MODE}
        --animated
        ${RAW_IMAGE_ANIM_ARGS}
    )
endfunction()

raw_audio(bgm_ambient out/sounds/ambient-bgm.wav)
raw_audio(bgm_menu out/sounds/menu-bgm.wav)

raw_image_alpha(default_boat out/textures/default-boat.png)
raw_image_alpha(sun out/textures/sun.png)
raw_image_alpha(moon out/textures/moon.png)
raw_image_alpha(compass_base out/textures/compass-base.png)
raw_image_alpha(compass_needle out/textures/compass-needle.png)
raw_image_alpha(crate out/textures/crate.png)
raw_image_animated(whirlpool out/textures/whirlpool.webp MODE argb8888)
raw_image_animated(water_texture out/textures/watertexture.webp)
raw_image(menu_bg out/textures/menu-bg.png)

bitmap_font(font_pixeloid_9px src/fonts/Pixeloid/TTF/PixeloidSans.ttf 9)
bitmap_font(font_pixeloid_9px_bold src/fonts/Pixeloid/TTF/PixeloidSans-Bold.ttf 9)
bitmap_font(font_pixeloid_18px src/fonts/Pixeloid/TTF/PixeloidSans.ttf 18)
bin2c_generic(bin2c_clouds.py bg_clouds out/textures/clouds.png)
