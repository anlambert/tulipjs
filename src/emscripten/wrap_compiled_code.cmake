FILE(READ ${JSFILE} EMSCRIPTEN_GENERATED_CODE)
FILE(READ ${JSUTILSFILE} JSUTILS_CODE)

FILE(WRITE ${JSFILE} "!function() {")
FILE(APPEND ${JSFILE} "  ${JSUTILS_CODE}")
FILE(APPEND ${JSFILE} "  ${EMSCRIPTEN_GENERATED_CODE}")
FILE(APPEND ${JSFILE} "  var tulip = {};")
FILE(APPEND ${JSFILE} "  this.tulip = tulip;")
FILE(APPEND ${JSFILE} "  var scriptName = getScriptName();")
FILE(APPEND ${JSFILE} "  var scriptPath = getScriptPath();")
FILE(APPEND ${JSFILE} "  tulip.filePackagePrefixURL = scriptPath;")
FILE(APPEND ${JSFILE} "  tulip.memoryInitializerPrefixURL = scriptPath;")
IF(TULIP_VIZ_FEATURES)
  FILE(APPEND ${JSFILE} "  tulip.vizFeatures = true;")
ELSE(TULIP_VIZ_FEATURES)
  FILE(APPEND ${JSFILE} "  tulip.vizFeatures = false;")
ENDIF(TULIP_VIZ_FEATURES)
IF(USE_WASM)
  FILE(APPEND ${JSFILE} "  var xhr = new XMLHttpRequest();")
  FILE(APPEND ${JSFILE} "  xhr.open('GET', scriptPath + 'tulip.wasm');")
  FILE(APPEND ${JSFILE} "  xhr.responseType = 'arraybuffer';")
  FILE(APPEND ${JSFILE} "  xhr.onload = function() {")
  FILE(APPEND ${JSFILE} "    tulip.wasmBinary = xhr.response;")
ENDIF(USE_WASM)
FILE(APPEND ${JSFILE} "    tulip = tulipjs(tulip);")
FILE(APPEND ${JSFILE} "    if (typeof define === 'function' && define.amd) define(tulip); else if (typeof module === 'object' && module.exports) module.exports = tulip;")
IF(USE_WASM)
  FILE(APPEND ${JSFILE} "  };")
  FILE(APPEND ${JSFILE} "  xhr.send(null);")
ENDIF(USE_WASM)
FILE(APPEND ${JSFILE} "}();")








