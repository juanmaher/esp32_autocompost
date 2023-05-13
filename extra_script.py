Import("env")

env.Append(LINKFLAGS=[
    "-Wl,--undefined=_binary_gtsr1_pem_start",
    "-Wl,--section-start=.my_section=0x84000"
])

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", 
    env.VerboseAction(" ".join([
        "$OBJCOPY",
        "-I", "binary",
        "-O", "elf32-littlearm",
        "--rename-section", ".data=.my_section",
        "${PROJECT_DATA_DIR}/src/esp_firebase/gtsr1.pem",
        "${BUILD_DIR}/src/esp_firebase/gtsr1.o"
    ]), "Creating binary ${TARGET}")
)

env.Append(LIBS=["${BUILD_DIR}/src/esp_firebase/gtsr1.o"])
