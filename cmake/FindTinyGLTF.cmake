find_path(TinyGLTF_INCLUDE_DIR
    NAMES tiny_gltf_v3.h
    HINTS "${TinyGLTF_ROOT}"
)

find_file(TinyGLTF_SOURCE_FILE
    NAMES tiny_gltf_v3.c
    HINTS "${TinyGLTF_INCLUDE_DIR}"
    NO_DEFAULT_PATH
)

find_file(TinyGLTF_JSON_HEADER
    NAMES tinygltf_json_c.h
    HINTS "${TinyGLTF_INCLUDE_DIR}"
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(TinyGLTF
    REQUIRED_VARS
        TinyGLTF_INCLUDE_DIR
        TinyGLTF_SOURCE_FILE
        TinyGLTF_JSON_HEADER
)