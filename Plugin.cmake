# ---- RNBO (C74) sources ---------------------------------------------
set(RNBO_JUCE_PARAM_DEFAULT_NOTIFY 1)
if (NOT PLUGIN_PARAM_DEFAULT_NOTIFY)
	set(RNBO_JUCE_PARAM_DEFAULT_NOTIFY 0)
endif()

set(RNBO_C74_SOURCES
  "${RNBO_CPP_DIR}/RNBO.cpp"
  "${RNBO_CPP_DIR}/adapters/juce/RNBO_JuceAudioProcessorEditor.cpp"
  "${RNBO_CPP_DIR}/adapters/juce/RNBO_JuceAudioProcessor.cpp"
  ${RNBO_CLASS_FILE}
)

add_library(RNBO_C74 STATIC ${RNBO_C74_SOURCES})

target_compile_features(RNBO_C74 PUBLIC cxx_std_17)

target_include_directories(RNBO_C74 PRIVATE
  ${RNBO_CPP_DIR}/
  ${RNBO_CPP_DIR}/src
  ${RNBO_CPP_DIR}/common/
  ${RNBO_CPP_DIR}/src/3rdparty/
)

# Use the same defs RNBO needs (copy any that are required by those files)
target_compile_definitions(RNBO_C74 PUBLIC
  RNBO_JUCE_NO_CREATE_PLUGIN_FILTER=1
  RNBO_JUCE_PARAM_DEFAULT_NOTIFY=${RNBO_JUCE_PARAM_DEFAULT_NOTIFY}
)

# Disable warnings ONLY for RNBO_C74
target_compile_options(RNBO_C74 PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/w>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
)

target_link_libraries(RNBO_C74
  PRIVATE
  juce::juce_audio_processors
  juce::juce_audio_formats
)

add_library(Stack STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stack/StackProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stack/StackEditor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/foleys_gui_magic/modules/foleys_gui_magic/foleys_gui_magic.cpp
 )

# Disable warnings for Stack (C74 code will otherwise emit warnings)
target_compile_options(Stack PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/w>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
)

target_include_directories(Stack PRIVATE
  ${RNBO_CPP_DIR}/
  ${RNBO_CPP_DIR}/src
  ${RNBO_CPP_DIR}/common/
  ${RNBO_CPP_DIR}/src/3rdparty/
  ${RNBO_CPP_DIR}/adapters/juce/
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/foleys_gui_magic/modules/foleys_gui_magic/

)

target_link_libraries(Stack
  PRIVATE
  RNBO_C74
  foleys_gui_magic
  juce::juce_audio_processors
  juce::juce_gui_basics
)


juce_add_plugin(MyPlugin
  # VERSION ...                        # Set this if the plugin version is different to the project version
  # ICON_BIG ...                       # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
  # ICON_SMALL ...
  COMPANY_NAME "Thompson"     # Specify the name of the plugin's author
  BUNDLE_ID "com.georgiasouthern.musictech"
  IS_SYNTH TRUE                        # Is this a synth or an effect?
  NEEDS_MIDI_INPUT TRUE                # Does the plugin need midi input?
  NEEDS_MIDI_OUTPUT FALSE               # Does the plugin need midi output?
  IS_MIDI_EFFECT FALSE                 # Is this plugin a MIDI effect?
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE    # Does the editor need keyboard focus?
  COPY_PLUGIN_AFTER_BUILD TRUE        # Should the plugin be installed to a default location after building?
  PLUGIN_MANUFACTURER_CODE "Gsou"      # A four-character manufacturer id with at least one upper-case character
  PLUGIN_CODE "St2k"                   # A unique four-character plugin id with at least one upper-case character
  FORMATS VST3 AU Standalone            # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
  PRODUCT_NAME "StackTest")          # The name of the final executable, which can differ from the target name


#juce_add_module(thirdparty/foleys_gui_magic/modules/foleys_gui_magic)

# Create magic.xml if it doesn't exist
set( MY_MAGIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/magic.xml" )
if(NOT EXISTS "${MY_MAGIC_SRC}")
    file(WRITE "${MY_MAGIC_SRC}" "")
    message(STATUS "Created empty magic.xml at ${MY_MAGIC_SRC}")
endif()

juce_add_binary_data(BinaryData
    SOURCES
    resources/magic.xml)

juce_generate_juce_header(MyPlugin)


target_sources(MyPlugin PRIVATE
  src/Plugin.cpp
  src/MyProcessor.cpp
  src/MyEditor.cpp
)

if (EXISTS ${RNBO_BINARY_DATA_FILE})
  target_sources(MyPlugin PRIVATE ${RNBO_BINARY_DATA_FILE})
endif()

target_include_directories(MyPlugin
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/
  src/
)

target_include_directories(MyPlugin SYSTEM PUBLIC
  ${RNBO_CPP_DIR}/
  ${RNBO_CPP_DIR}/src
  ${RNBO_CPP_DIR}/common/
  ${RNBO_CPP_DIR}/src/3rdparty/
  ${RNBO_CPP_DIR}/adapters/juce/
)

set(RNBO_JUCE_PARAM_DEFAULT_NOTIFY 1)
if (NOT PLUGIN_PARAM_DEFAULT_NOTIFY)
	set(RNBO_JUCE_PARAM_DEFAULT_NOTIFY 0)
endif()

target_compile_definitions(MyPlugin
  PUBLIC
  # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
  JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
  JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
  JUCE_VST3_CAN_REPLACE_VST2=0
  RNBO_JUCE_NO_CREATE_PLUGIN_FILTER=1 #don't have RNBO create its own createPluginFilter function, we'll create it ourselves
  RNBO_JUCE_PARAM_DEFAULT_NOTIFY=${RNBO_JUCE_PARAM_DEFAULT_NOTIFY}
  JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:RNBOApp,JUCE_VERSION>"
  MY_MAGIC_SRC="${MY_MAGIC_SRC}"
)

target_link_libraries(MyPlugin
  PRIVATE
  juce::juce_audio_utils
  Stack
  foleys_gui_magic
  BinaryData
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)

#TODO windows and linux
if(APPLE)
  install(
    TARGETS RNBOAudioPlugin_VST3
    DESTINATION ~/Library/Audio/Plug-Ins/VST3/
    )
  install(
    TARGETS RNBOAudioPlugin_AU
    DESTINATION ~/Library/Audio/Plug-Ins/Components/
    )
endif()
