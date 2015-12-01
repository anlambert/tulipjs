var UINT_MAX = 0xffffffff;

function typeOf(value) {
  var s = typeof value;
  if (s === 'object') {
    if (value) {
      if (Object.prototype.toString.call(value) == '[object Array]') {
        s = 'array';
      }
    } else {
      s = 'null';
    }
  }
  return s;
}

function _allocArrayInEmHeap(ArrayType, size) {
  var nDataBytes = size * ArrayType.BYTES_PER_ELEMENT;
  var dataPtr = Module._malloc(nDataBytes);
  return new ArrayType(Module.HEAPU8.buffer, dataPtr, size);
}

function _freeArrayInEmHeap(arrayHeap) {
  Module._free(arrayHeap.byteOffset);
}

function _bytesTypedArrayToStringArray(bytesArray, offsetArray, nbStrings) {
  var ret = [];
  var start = 0;
  for (var i = 0 ; i < nbStrings ; ++i) {
    ret.push(Module.UTF8ArrayToString(bytesArray.subarray(start, start+offsetArray[i]), 0));
    start += offsetArray[i];
  }
  return ret;
}

function _stringArrayToBytesAndOffsetsTypedArray(stringArray) {
  var nbBytes = 0;
  var uintArray = _allocArrayInEmHeap(Uint32Array, stringArray.length);
  for (var i = 0 ; i < stringArray.length ; ++i) {
    var strNbBytes = Module.lengthBytesUTF8(stringArray[i]) + 1;
    nbBytes += strNbBytes;
    uintArray[i] = strNbBytes;
  }
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  var offset = 0;
  for (var i = 0 ; i < stringArray.length ; ++i) {
    Module.stringToUTF8Array(stringArray[i], ucharArray, offset, uintArray[i]);
    offset += uintArray[i];
  }
  return {bytesArray: ucharArray, offsetsArray: uintArray};
}

Function.prototype.inheritsFrom = function(parentClassOrObject) {
  this.prototype = Object.create(parentClassOrObject.prototype);
  this.prototype.constructor = this;
}

if (typeOf(String.prototype.startsWith) == 'undefined') {

  String.prototype.startsWith = function(prefix) {
    return this.indexOf(prefix) === 0;
  }

  String.prototype.endsWith = function(suffix) {
    return this.match(suffix+"$") == suffix;
  };

}

function createObject(ObjectType, caller) {
  return ObjectType.prototype.isPrototypeOf(caller) ? caller : new ObjectType();
}

var tulip = Module;

tulip.mainCalled = false;
if (nodejs) {
  tulip.mainCalled = true;
}

tulip.debugChecks = tulip.debugChecks || true;
if (workerMode) {
  tulip.debugChecks = false;
}

function checkArgumentsTypes(argList, typeList, nbRequiredArguments) {
  if (!tulip.debugChecks) return;
  var callerName = arguments.callee.caller.name;
  if (callerName.startsWith("tulip_")) {
    callerName = callerName.replace(/_/g, '.');
  }
  var nbArgs = argList.length;
  if (nbRequiredArguments && nbRequiredArguments > nbArgs) nbArgs = nbRequiredArguments;
  for (var i = 0 ; i < nbArgs ; ++i) {
    if (typeOf(typeList[i]) == "string" && typeOf(argList[i]) != typeList[i]) {
      throw new TypeError("Error when calling function '" + callerName + "', parameter "+ i + " must be of type '"+ typeList[i] +"' (found '" + typeOf(argList[i]) + "' instead)");
    } else if (typeOf(typeList[i]) == "function") {
      if (typeOf(argList[i]) != "object" || !(argList[i] instanceof typeList[i])) {
        var typename = typeList[i].name;
        if (typename.startsWith("tulip_")) {
          typename = typename.replace(/_/g, '.');
        }
        throw new TypeError("Error when calling function '" + callerName + "', parameter "+ i + " must be an instance of a '"+ typename +"' object (found '" + typeOf(argList[i]) + "' instead)");
      }
    } else if (typeOf(typeList[i]) == "array") {
      var typeOk = false;
      for (var j = 0 ; j < typeList[i].length ; ++j) {
        if (typeOf(typeList[i][j]) == "string" && typeOf(argList[i]) == typeList[i][j]) {
          typeOk = true;
          break;
        } else if (typeOf(typeList[i][j]) == "function") {
          if (typeOf(argList[i]) == "object" && argList[i] instanceof typeList[i][j]) {
            typeOk = true;
            break;
          }
        }
      }
      if (!typeOk) {
        var errMsg = "Error when calling function '" + callerName + "', parameter "+ i + " must be of one of the following types : ";
        for (var j = 0 ; j < typeList[i].length ; ++j) {
          if (typeOf(typeList[i][j]) == "string") {
            errMsg += typeList[i][j];
          } else if (typeOf(typeList[i][j]) == "function") {
            var typename = typeList[i][j].name;
            if (typename.startsWith("tulip_")) {
              typename = typename.replace(/_/g, '.');
            }
            errMsg += "an instance of a '"+ typename +"' object";
          }
          if (j != typeList[i].length - 1) {
            errMsg += ", ";
          }
        }
        errMsg += ", found '" + typeOf(argList[i]) + "' instead";
        throw new TypeError(errMsg);
      }
    }
  }
}

function checkArrayOfType(array, type, i) {
  if (!tulip.debugChecks) return;
  var callerName = arguments.callee.caller.name;
  if (callerName.startsWith("tulip_")) {
    callerName = callerName.replace(/_/g, '.');
  }
  var types = [];
  for (var j = 0 ; j < array.length ; ++j) {
    types.push(type);
  }
  try {
    checkArgumentsTypes(array, types);
  } catch (e) {
    var typename = type;
    if (typeOf(typename) == "function") {
      typename = type.name;
      if (typename.startsWith("tulip_")) {
        typename = typename.replace(/_/g, '.');
      }
    }
    throw new TypeError("Error when calling function '" + callerName + "', parameter "+ i + " must be an array of "+ typename);
  }
}

var _isPointerDeleted = Module.cwrap('isPointerDeleted', 'number', ['number']);

function checkWrappedCppPointer(cppPointer) {
  //if (!tulip.debugChecks) return;
  if (cppPointer == 0 || _isPointerDeleted(cppPointer)) {
    var callerName = arguments.callee.caller.name;
    if (callerName.startsWith("tulip_")) {
      callerName = callerName.replace(/_/g, '.');
    }
    throw "Runtime error when calling function '" + callerName +"' : wrapped C++ object is null or has been deleted";
  }
}

function getArrayOfTulipType(arraySize, arrayFillFunc, tulipType) {
  var result = _allocArrayInEmHeap(Uint32Array, arraySize);
  arrayFillFunc(result.byteOffset);
  var tulipTypeArray = new Array();
  for (var i = 0 ; i < arraySize ; ++i) {
    if (tulipType == tulip.Node || tulipType == tulip.Edge) {
      if (result[i] == UINT_MAX) {
        break;
      }
    }
    tulipTypeArray.push(tulipType(result[i]));
  }
  _freeArrayInEmHeap(result);
  return tulipTypeArray;
};

tulip.CppObjectWrapper = function(cppPointer, wrappedTypename) {
  this.cppPointer = cppPointer;
  this.wrappedTypename = wrappedTypename;
};

tulip.CppObjectWrapper.prototype.getCppPointer = function() {
  return this.cppPointer;
};

tulip.CppObjectWrapper.prototype.cppPointerValid = function() {
  return this.cppPointer != 0 && !_isPointerDeleted(this.cppPointer);
};

tulip.CppObjectWrapper.prototype.getWrappedTypename = function() {
  return this.wrappedTypename;
};

tulip.CppObjectWrapper.prototype.setWrappedTypename = function(typename) {
  this.wrappedTypename = typename;
};

// ==================================================================================================================

var _PropertyInterface_delete = Module.cwrap('PropertyInterface_delete', null, ["number"]);
var _PropertyInterface_getName = Module.cwrap('PropertyInterface_getName', 'string', ['number']);
var _PropertyInterface_getTypename = Module.cwrap('PropertyInterface_getTypename', 'string', ['number']);
var _PropertyInterface_getGraph = Module.cwrap('PropertyInterface_getGraph', 'number', ['number']);
var _PropertyInterface_getNodeStringValue = Module.cwrap('PropertyInterface_getNodeStringValue', 'string', ['number', 'number']);
var _PropertyInterface_getEdgeStringValue = Module.cwrap('PropertyInterface_getEdgeStringValue', 'string', ['number', 'number']);

/**
* This is the description for the tulip.node class.
*
* @class PropertyInterface
*/
tulip.PropertyInterface = function tulip_PropertyInterface(cppPointer, graphManaged) {
  var newObject = createObject(tulip.PropertyInterface, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    tulip.CppObjectWrapper.call(newObject, cppPointer, "tlp::PropertyInterface");
    newObject.graphManaged = graphManaged;
  }
  return newObject;
};
tulip.PropertyInterface.inheritsFrom(tulip.CppObjectWrapper);
tulip.PropertyInterface.prototype.destroy = function tulip_PropertyInterface_prototype_destroy() {
  checkWrappedCppPointer(this.cppPointer);
  if (!this.graphManaged) {
    _PropertyInterface_delete(this.cppPointer);
    this.cppPointer = 0;
  } else {
    console.log("Not destroying property named '" + this.getName() + "' as it is managed by the graph named '" + this.getGraph().getName() + "'");
  }
};
tulip.PropertyInterface.prototype.getName = function tulip_PropertyInterface_prototype_getName() {
  checkWrappedCppPointer(this.cppPointer);
  return _PropertyInterface_getName(this.cppPointer);
};
tulip.PropertyInterface.prototype.getTypename = function tulip_PropertyInterface_prototype_getTypename() {
  checkWrappedCppPointer(this.cppPointer);
  return _PropertyInterface_getTypename(this.cppPointer);
};
tulip.PropertyInterface.prototype.getNodeStringValue = function tulip_PropertyInterface_prototype_getNodeStringValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node], 1);
  return _PropertyInterface_getNodeStringValue(this.cppPointer, node.id);
};
tulip.PropertyInterface.prototype.getEdgeStringValue = function tulip_PropertyInterface_prototype_getEdgeStringValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge], 1);
  return _PropertyInterface_getEdgeStringValue(this.cppPointer, edge.id);
};
/**
* Returns the graph that have created that property
*
* @method getGraph
* @return {Graph}
*/
tulip.PropertyInterface.prototype.getGraph = function tulip_PropertyInterface_prototype_getGraph() {
  checkWrappedCppPointer(this.cppPointer);
  return tulip.Graph(_PropertyInterface_getGraph(this.cppPointer));
}

// ==================================================================================================================

var _createBooleanProperty = Module.cwrap('createBooleanProperty', 'number', ['number', 'string']);
var _BooleanProperty_setAllNodeValue = Module.cwrap('BooleanProperty_setAllNodeValue', null, ['number', 'number']);
var _BooleanProperty_getNodeDefaultValue = Module.cwrap('BooleanProperty_getNodeDefaultValue', 'number', ['number']);
var _BooleanProperty_setNodeValue = Module.cwrap('BooleanProperty_setNodeValue', null, ['number', 'number', 'number']);
var _BooleanProperty_getNodeValue = Module.cwrap('BooleanProperty_getNodeValue', 'number', ['number', 'number']);
var _BooleanProperty_setAllEdgeValue = Module.cwrap('BooleanProperty_setAllEdgeValue', null, ['number', 'number']);
var _BooleanProperty_setEdgeValue = Module.cwrap('BooleanProperty_setEdgeValue', null, ['number', 'number', 'number']);
var _BooleanProperty_getEdgeDefaultValue = Module.cwrap('BooleanProperty_getEdgeDefaultValue', 'number', ['number']);
var _BooleanProperty_getEdgeValue = Module.cwrap('BooleanProperty_getEdgeValue', 'number', ['number', 'number']);
var _BooleanProperty_getNodesEqualTo = Module.cwrap('BooleanProperty_getNodesEqualTo', null, ['number', 'number', 'number', 'number']);
var _BooleanProperty_getEdgesEqualTo = Module.cwrap('BooleanProperty_getEdgesEqualTo', null, ['number', 'number', 'number', 'number']);

tulip.BooleanProperty = function tulip_BooleanProperty() {
  var newObject = createObject(tulip.BooleanProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createBooleanProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::BooleanProperty");
  }
  return newObject;
};
tulip.BooleanProperty.inheritsFrom(tulip.PropertyInterface);
tulip.BooleanProperty.prototype.getNodeDefaultValue = function tulip_BooleanProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _BooleanProperty_getNodeDefaultValue(this.cppPointer) > 0;
};
tulip.BooleanProperty.prototype.getNodeValue = function tulip_BooleanProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  return _BooleanProperty_getNodeValue(this.cppPointer, node.id) > 0;
};
tulip.BooleanProperty.prototype.setNodeValue = function tulip_BooleanProperty_prototype_setNodeValue(node, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "boolean"]);
  _BooleanProperty_setNodeValue(this.cppPointer, node.id, val);
};
tulip.BooleanProperty.prototype.getEdgeDefaultValue = function tulip_BooleanProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _BooleanProperty_getEdgeDefaultValue(this.cppPointer) > 0;
};
tulip.BooleanProperty.prototype.getEdgeValue = function tulip_BooleanProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  return _BooleanProperty_getEdgeValue(this.cppPointer, edge.id) > 0;
};
tulip.BooleanProperty.prototype.setEdgeValue = function tulip_BooleanProperty_prototype_setEdgeValue(edge, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "boolean"]);
  _BooleanProperty_setEdgeValue(this.cppPointer, edge.id, val);
};
tulip.BooleanProperty.prototype.setAllNodeValue = function tulip_BooleanProperty_prototype_setAllNodeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["boolean"]);
  _BooleanProperty_setAllNodeValue(this.cppPointer, val);
};
tulip.BooleanProperty.prototype.setAllEdgeValue = function tulip_BooleanProperty_prototype_setAllEdgeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["boolean"]);
  _BooleanProperty_setAllEdgeValue(this.cppPointer, val);
};
tulip.BooleanProperty.prototype.getNodesEqualTo = function tulip_BooleanProperty_prototype_getNodesEqualTo(val, graph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["boolean", tulip.Graph]);
  var propObject = this;
  return getArrayOfTulipType(propObject.getGraph().numberOfNodes(), function(byteOffset){_BooleanProperty_getNodesEqualTo(propObject.cppPointer, val, graph ? graph.cppPointer : 0, byteOffset)}, tulip.Node);
};
tulip.BooleanProperty.prototype.getEdgesEqualTo = function tulip_BooleanProperty_prototype_getEdgesEqualTo(val, graph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["boolean", tulip.Graph]);
  var propObject = this;
  return getArrayOfTulipType(propObject.getGraph().numberOfEdges(), function(byteOffset){_BooleanProperty_getEdgesEqualTo(propObject.cppPointer, val, graph ? graph.cppPointer : 0, byteOffset)}, tulip.Edge);
};
// ==================================================================================================================

var _createDoubleProperty = Module.cwrap('createDoubleProperty', 'number', ['number', 'string']);
var _DoubleProperty_setAllNodeValue = Module.cwrap('DoubleProperty_setAllNodeValue', null, ['number', 'number']);
var _DoubleProperty_setNodeValue = Module.cwrap('DoubleProperty_setNodeValue', null, ['number', 'number', 'number']);
var _DoubleProperty_getNodeDefaultValue = Module.cwrap('DoubleProperty_getNodeDefaultValue', 'number', ['number']);
var _DoubleProperty_getNodeValue = Module.cwrap('DoubleProperty_getNodeValue', 'number', ['number', 'number']);
var _DoubleProperty_setAllEdgeValue = Module.cwrap('DoubleProperty_setAllEdgeValue', null, ['number', 'number']);
var _DoubleProperty_setEdgeValue = Module.cwrap('DoubleProperty_setEdgeValue', null, ['number', 'number', 'number']);
var _DoubleProperty_getEdgeDefaultValue = Module.cwrap('DoubleProperty_getEdgeDefaultValue', 'number', ['number']);
var _DoubleProperty_getEdgeValue = Module.cwrap('DoubleProperty_getEdgeValue', 'number', ['number', 'number']);
var _DoubleProperty_getSortedEdges = Module.cwrap('DoubleProperty_getSortedEdges', null, ['number', 'number', 'number', 'number']);

tulip.DoubleProperty = function tulip_DoubleProperty() {
  var newObject = createObject(tulip.DoubleProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createDoubleProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::DoubleProperty");
  }
  return newObject;
};
tulip.DoubleProperty.inheritsFrom(tulip.PropertyInterface);
tulip.DoubleProperty.prototype.getNodeDefaultValue = function tulip_DoubleProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _DoubleProperty_getNodeDefaultValue(this.cppPointer);
};
tulip.DoubleProperty.prototype.getNodeValue = function tulip_DoubleProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  return _DoubleProperty_getNodeValue(this.cppPointer, node.id);
};
tulip.DoubleProperty.prototype.setNodeValue = function tulip_DoubleProperty_prototype_setNodeValue(node, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "number"]);
  _DoubleProperty_setNodeValue(this.cppPointer, node.id, val);
};
tulip.DoubleProperty.prototype.getEdgeDefaultValue = function tulip_DoubleProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _DoubleProperty_getEdgeDefaultValue(this.cppPointer);
};
tulip.DoubleProperty.prototype.getEdgeValue = function tulip_DoubleProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  return _DoubleProperty_getEdgeValue(this.cppPointer, edge.id);
};
tulip.DoubleProperty.prototype.setEdgeValue = function tulip_DoubleProperty_prototype_setEdgeValue(edge, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "number"]);
  _DoubleProperty_setEdgeValue(this.cppPointer, edge.id, val);
};
tulip.DoubleProperty.prototype.setAllNodeValue = function tulip_DoubleProperty_prototype_setAllNodeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["number"]);
  _DoubleProperty_setAllNodeValue(this.cppPointer, val);
};
tulip.DoubleProperty.prototype.setAllEdgeValue = function tulip_DoubleProperty_prototype_setAllEdgeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["number"]);
  _DoubleProperty_setAllEdgeValue(this.cppPointer, val);
};
tulip.DoubleProperty.prototype.getSortedEdges = function tulip_DoubleProperty_prototype_getSortedEdges(sg, ascendingOrder) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Graph, "boolean"]);
  if (typeOf(sg) == "undefined") sg = this.getGraph();
  if (typeOf(ascendingOrder) == "undefined") ascendingOrder = true;
  var propertyObject = this;
  return getArrayOfTulipType(sg.numberOfEdges(), function(byteOffset){_DoubleProperty_getSortedEdges(propertyObject.cppPointer, sg.cppPointer, ascendingOrder, byteOffset)}, tulip.Edge);
};
// ==================================================================================================================

var _createDoubleVectorProperty = Module.cwrap('createDoubleVectorProperty', 'number', ['number', 'string']);

var _DoubleVectorProperty_setAllNodeValue = Module.cwrap('DoubleVectorProperty_setAllNodeValue', null, ['number', 'number', 'number']);
var _DoubleVectorProperty_setNodeValue = Module.cwrap('DoubleVectorProperty_setNodeValue', null, ['number', 'number', 'number', 'number']);
var _DoubleVectorProperty_getNodeDefaultVectorSize = Module.cwrap('DoubleVectorProperty_getNodeDefaultVectorSize', 'number', ['number']);
var _DoubleVectorProperty_getNodeDefaultValue = Module.cwrap('DoubleVectorProperty_getNodeDefaultValue', null, ['number', 'number']);
var _DoubleVectorProperty_getNodeVectorSize = Module.cwrap('DoubleVectorProperty_getNodeVectorSize', 'number', ['number', 'number']);
var _DoubleVectorProperty_getNodeValue = Module.cwrap('DoubleVectorProperty_getNodeValue', null, ['number', 'number', 'number']);

var _DoubleVectorProperty_setAllEdgeValue = Module.cwrap('DoubleVectorProperty_setAllEdgeValue', null, ['number', 'number', 'number']);
var _DoubleVectorProperty_setEdgeValue = Module.cwrap('DoubleVectorProperty_setEdgeValue', null, ['number', 'number', 'number', 'number']);
var _DoubleVectorProperty_getEdgeDefaultVectorSize = Module.cwrap('DoubleVectorProperty_getEdgeDefaultVectorSize', 'number', ['number']);
var _DoubleVectorProperty_getEdgeDefaultValue = Module.cwrap('DoubleVectorProperty_getEdgeDefaultValue', null, ['number', 'number']);
var _DoubleVectorProperty_getEdgeVectorSize = Module.cwrap('DoubleVectorProperty_getEdgeVectorSize', 'number', ['number', 'number']);
var _DoubleVectorProperty_getEdgeValue = Module.cwrap('DoubleVectorProperty_getEdgeValue', null, ['number', 'number', 'number']);

tulip.DoubleVectorProperty = function tulip_DoubleVectorProperty() {
  var newObject = createObject(tulip.DoubleVectorProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createDoubleVectorProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::DoubleVectorProperty");
  }
  return newObject;
};
tulip.DoubleVectorProperty.inheritsFrom(tulip.PropertyInterface);
tulip.DoubleVectorProperty.prototype.getNodeDefaultValue = function tulip_DoubleVectorProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var size = _DoubleVectorProperty_getNodeDefaultVectorSize(this.cppPointer);
  var doubleArray = _allocArrayInEmHeap(Float64Array, size);
  _DoubleVectorProperty_getNodeDefaultValue(this.cppPointer, doubleArray.byteOffset);
  var ret = Array.prototype.slice.call(doubleArray);
  _freeArrayInEmHeap(doubleArray);
  return ret;
};
tulip.DoubleVectorProperty.prototype.getNodeValue = function tulip_DoubleVectorProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node], 1);
  var size = _DoubleVectorProperty_getNodeVectorSize(this.cppPointer, node.id);
  var doubleArray = _allocArrayInEmHeap(Float64Array, size);
  _DoubleVectorProperty_getNodeValue(this.cppPointer, node.id, doubleArray.byteOffset);
  var ret = Array.prototype.slice.call(doubleArray);
  _freeArrayInEmHeap(doubleArray);
  return ret;
};
tulip.DoubleVectorProperty.prototype.setNodeValue = function tulip_DoubleVectorProperty_prototype_setNodeValue(node, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "array"], 2);
  checkArrayOfType(val, "number", 1);
  var doubleArray = _allocArrayInEmHeap(Float64Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    doubleArray[i] = val[i];
  }
  _DoubleVectorProperty_setNodeValue(this.cppPointer, node.id, doubleArray.byteOffset, val.length);
  _freeArrayInEmHeap(doubleArray);
};
tulip.DoubleVectorProperty.prototype.getEdgeDefaultValue = function tulip_DoubleVectorProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var size = _DoubleVectorProperty_getEdgeDefaultVectorSize(this.cppPointer);
  var doubleArray = _allocArrayInEmHeap(Float64Array, size);
  _DoubleVectorProperty_getEdgeDefaultValue(this.cppPointer, doubleArray.byteOffset);
  var ret = Array.prototype.slice.call(doubleArray);
  _freeArrayInEmHeap(doubleArray);
  return ret;
};
tulip.DoubleVectorProperty.prototype.getEdgeValue = function tulip_DoubleVectorProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge], 1);
  var size = _DoubleVectorProperty_getEdgeVectorSize(this.cppPointer, edge.id);
  var doubleArray = _allocArrayInEmHeap(Float64Array, size);
  _DoubleVectorProperty_getEdgeValue(this.cppPointer, edge.id, doubleArray.byteOffset);
  var ret = Array.prototype.slice.call(doubleArray);
  _freeArrayInEmHeap(doubleArray);
  return ret;
};
tulip.DoubleVectorProperty.prototype.setEdgeValue = function tulip_DoubleVectorProperty_prototype_setEdgeValue(edge, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "array"], 2);
  checkArrayOfType(val, "number", 1);
  var doubleArray = _allocArrayInEmHeap(Float64Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    doubleArray[i] = val[i];
  }
  _DoubleVectorProperty_setEdgeValue(this.cppPointer, edge.id, doubleArray.byteOffset, val.length);
  _freeArrayInEmHeap(doubleArray);
};
tulip.DoubleVectorProperty.prototype.setAllNodeValue = function tulip_DoubleVectorProperty_prototype_setAllNodeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"], 1);
  checkArrayOfType(val, "number", 0);
  var doubleArray = _allocArrayInEmHeap(Float64Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    doubleArray[i] = val[i];
  }
  _DoubleVectorProperty_setAllNodeValue(this.cppPointer, doubleArray.byteOffset, val.length);
  _freeArrayInEmHeap(doubleArray);
};
tulip.DoubleVectorProperty.prototype.setAllEdgeValue = function tulip_DoubleVectorProperty_prototype_setAllEdgeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"], 1);
  checkArrayOfType(val, "number", 0);
  var doubleArray = _allocArrayInEmHeap(Float64Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    doubleArray[i] = val[i];
  }
  _DoubleVectorProperty_setAllEdgeValue(this.cppPointer, doubleArray.byteOffset, val.length);
  _freeArrayInEmHeap(doubleArray);
};

// ==================================================================================================================

var _createIntegerProperty = Module.cwrap('createIntegerProperty', 'number', ['number', 'string']);
var _IntegerProperty_setAllNodeValue = Module.cwrap('IntegerProperty_setAllNodeValue', null, ['number', 'number']);
var _IntegerProperty_setNodeValue = Module.cwrap('IntegerProperty_setNodeValue', null, ['number', 'number', 'number']);
var _IntegerProperty_getNodeDefaultValue = Module.cwrap('IntegerProperty_getNodeDefaultValue', 'number', ['number']);
var _IntegerProperty_getNodeValue = Module.cwrap('IntegerProperty_getNodeValue', 'number', ['number', 'number']);
var _IntegerProperty_setAllEdgeValue = Module.cwrap('IntegerProperty_setAllEdgeValue', null, ['number', 'number']);
var _IntegerProperty_setEdgeValue = Module.cwrap('IntegerProperty_setEdgeValue', null, ['number', 'number', 'number']);
var _IntegerProperty_getEdgeDefaultValue = Module.cwrap('IntegerProperty_getEdgeDefaultValue', 'number', ['number']);
var _IntegerProperty_getEdgeValue = Module.cwrap('IntegerProperty_getEdgeValue', 'number', ['number', 'number']);

tulip.IntegerProperty = function tulip_IntegerProperty() {
  var newObject = createObject(tulip.IntegerProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createIntegerProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::IntegerProperty");
  }
  return newObject;
};
tulip.IntegerProperty.inheritsFrom(tulip.PropertyInterface);
tulip.IntegerProperty.prototype.getNodeDefaultValue = function tulip_IntegerProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _IntegerProperty_getNodeDefaultValue(this.cppPointer);
};
tulip.IntegerProperty.prototype.getNodeValue = function tulip_IntegerProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  return _IntegerProperty_getNodeValue(this.cppPointer, node.id);
};
tulip.IntegerProperty.prototype.setNodeValue = function tulip_IntegerProperty_prototype_setNodeValue(node, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "number"]);
  _IntegerProperty_setNodeValue(this.cppPointer, node.id, val);
};
tulip.IntegerProperty.prototype.getEdgeDefaultValue = function tulip_IntegerProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _IntegerProperty_getEdgeDefaultValue(this.cppPointer);
};
tulip.IntegerProperty.prototype.getEdgeValue = function tulip_IntegerProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  return _IntegerProperty_getEdgeValue(this.cppPointer, edge.id);
};
tulip.IntegerProperty.prototype.setEdgeValue = function tulip_IntegerProperty_prototype_setEdgeValue(edge, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "number"]);
  _IntegerProperty_setEdgeValue(this.cppPointer, edge.id, val);
};
tulip.IntegerProperty.prototype.setAllNodeValue = function tulip_IntegerProperty_prototype_setAllNodeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["number"]);
  _IntegerProperty_setAllNodeValue(this.cppPointer, val);
};
tulip.IntegerProperty.prototype.setAllEdgeValue = function tulip_IntegerProperty_prototype_setAllEdgeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["number"]);
  _IntegerProperty_setAllEdgeValue(this.cppPointer, val);
};

// ==================================================================================================================

var _createIntegerVectorProperty = Module.cwrap('createIntegerVectorProperty', 'number', ['number', 'string']);

var _IntegerVectorProperty_setAllNodeValue = Module.cwrap('IntegerVectorProperty_setAllNodeValue', null, ['number', 'number', 'number']);
var _IntegerVectorProperty_setNodeValue = Module.cwrap('IntegerVectorProperty_setNodeValue', null, ['number', 'number', 'number', 'number']);
var _IntegerVectorProperty_getNodeDefaultVectorSize = Module.cwrap('IntegerVectorProperty_getNodeDefaultVectorSize', 'number', ['number']);
var _IntegerVectorProperty_getNodeDefaultValue = Module.cwrap('IntegerVectorProperty_getNodeDefaultValue', null, ['number', 'number']);
var _IntegerVectorProperty_getNodeVectorSize = Module.cwrap('IntegerVectorProperty_getNodeVectorSize', 'number', ['number', 'number']);
var _IntegerVectorProperty_getNodeValue = Module.cwrap('IntegerVectorProperty_getNodeValue', null, ['number', 'number', 'number']);

var _IntegerVectorProperty_setAllEdgeValue = Module.cwrap('IntegerVectorProperty_setAllEdgeValue', null, ['number', 'number', 'number']);
var _IntegerVectorProperty_setEdgeValue = Module.cwrap('IntegerVectorProperty_setEdgeValue', null, ['number', 'number', 'number', 'number']);
var _IntegerVectorProperty_getEdgeDefaultVectorSize = Module.cwrap('IntegerVectorProperty_getEdgeDefaultVectorSize', 'number', ['number']);
var _IntegerVectorProperty_getEdgeDefaultValue = Module.cwrap('IntegerVectorProperty_getEdgeDefaultValue', null, ['number', 'number']);
var _IntegerVectorProperty_getEdgeVectorSize = Module.cwrap('IntegerVectorProperty_getEdgeVectorSize', 'number', ['number', 'number']);
var _IntegerVectorProperty_getEdgeValue = Module.cwrap('IntegerVectorProperty_getEdgeValue', null, ['number', 'number', 'number']);

tulip.IntegerVectorProperty = function tulip_IntegerVectorProperty() {
  var newObject = createObject(tulip.IntegerVectorProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createIntegerVectorProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::IntegerVectorProperty");
  }
  return newObject;
};
tulip.IntegerVectorProperty.inheritsFrom(tulip.PropertyInterface);
tulip.IntegerVectorProperty.prototype.getNodeDefaultValue = function tulip_IntegerVectorProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var size = _IntegerVectorProperty_getNodeDefaultVectorSize(this.cppPointer);
  var integerArray = _allocArrayInEmHeap(Int32Array, size);
  _IntegerVectorProperty_getNodeDefaultValue(this.cppPointer, integerArray.byteOffset);
  var ret = Array.prototype.slice.call(integerArray);
  _freeArrayInEmHeap(integerArray);
  return ret;
};
tulip.IntegerVectorProperty.prototype.getNodeValue = function tulip_IntegerVectorProperty_prototype_getNodeValue2(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node], 1);
  var size = _IntegerVectorProperty_getNodeVectorSize(this.cppPointer, node.id);
  var integerArray = _allocArrayInEmHeap(Int32Array, size);
  _IntegerVectorProperty_getNodeValue(this.cppPointer, node.id, integerArray.byteOffset);
  var ret = Array.prototype.slice.call(integerArray);
  _freeArrayInEmHeap(integerArray);
  return ret;
};
tulip.IntegerVectorProperty.prototype.setNodeValue = function tulip_IntegerVectorProperty_prototype_setNodeValue(node, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "array"], 2);
  checkArrayOfType(val, "number", 1);
  var integerArray = _allocArrayInEmHeap(Int32Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    integerArray[i] = val[i];
  }
  _IntegerVectorProperty_setNodeValue(this.cppPointer, node.id, integerArray.byteOffset, val.length);
  _freeArrayInEmHeap(integerArray);
};
tulip.IntegerVectorProperty.prototype.getEdgeDefaultValue = function tulip_IntegerVectorProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var size = _IntegerVectorProperty_getEdgeDefaultVectorSize(this.cppPointer);
  var integerArray = _allocArrayInEmHeap(Int32Array, size);
  _IntegerVectorProperty_getEdgeDefaultValue(this.cppPointer, integerArray.byteOffset);
  var ret = Array.prototype.slice.call(integerArray);
  _freeArrayInEmHeap(integerArray);
  return ret;
};
tulip.IntegerVectorProperty.prototype.getEdgeValue = function tulip_IntegerVectorProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge], 1);
  var size = _IntegerVectorProperty_getEdgeVectorSize(this.cppPointer, edge.id);
  var integerArray = _allocArrayInEmHeap(Int32Array, size);
  _IntegerVectorProperty_getEdgeValue(this.cppPointer, edge.id, integerArray.byteOffset);
  var ret = Array.prototype.slice.call(integerArray);
  _freeArrayInEmHeap(integerArray);
  return ret;
};
tulip.IntegerVectorProperty.prototype.setEdgeValue = function tulip_IntegerVectorProperty_prototype_setEdgeValue(edge, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "array"], 2);
  checkArrayOfType(val, "number", 1);
  var integerArray = _allocArrayInEmHeap(Int32Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    integerArray[i] = val[i];
  }
  _IntegerVectorProperty_setEdgeValue(this.cppPointer, edge.id, integerArray.byteOffset, val.length);
  _freeArrayInEmHeap(integerArray);
};
tulip.IntegerVectorProperty.prototype.setAllNodeValue = function tulip_IntegerVectorProperty_prototype_setAllNodeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"], 1);
  checkArrayOfType(val, "number", 0);
  var integerArray = _allocArrayInEmHeap(Int32Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    integerArray[i] = val[i];
  }
  _IntegerVectorProperty_setAllNodeValue(this.cppPointer, integerArray.byteOffset, val.length);
  _freeArrayInEmHeap(integerArray);
};
tulip.IntegerVectorProperty.prototype.setAllEdgeValue = function tulip_IntegerVectorProperty_prototype_setAllEdgeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"], 1);
  checkArrayOfType(val, "number", 0);
  var integerArray = _allocArrayInEmHeap(Int32Array, val.length);
  for (var i = 0 ; i < val.length ; ++i) {
    integerArray[i] = val[i];
  }
  _IntegerVectorProperty_setAllEdgeValue(this.cppPointer, integerArray.byteOffset, val.length);
  _freeArrayInEmHeap(integerArray);
};

// ==================================================================================================================

var _createStringProperty = Module.cwrap('createStringProperty', 'number', ['number', 'string']);
var _StringProperty_setAllNodeValue = Module.cwrap('StringProperty_setAllNodeValue', null, ['number', 'string']);
var _StringProperty_setNodeValue = Module.cwrap('StringProperty_setNodeValue', null, ['number', 'number', 'string']);
var _StringProperty_getNodeDefaultValue = Module.cwrap('StringProperty_getNodeDefaultValue', 'string', ['number']);
var _StringProperty_getNodeValue = Module.cwrap('StringProperty_getNodeValue', 'string', ['number', 'number']);
var _StringProperty_setAllEdgeValue = Module.cwrap('StringProperty_setAllEdgeValue', null, ['number', 'string']);
var _StringProperty_setEdgeValue = Module.cwrap('StringProperty_setEdgeValue', null, ['number', 'number', 'string']);
var _StringProperty_getEdgeDefaultValue = Module.cwrap('StringProperty_getEdgeDefaultValue', 'string', ['number']);
var _StringProperty_getEdgeValue = Module.cwrap('StringProperty_getEdgeValue', 'string', ['number', 'number']);

tulip.StringProperty = function tulip_StringProperty() {
  var newObject = createObject(tulip.StringProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createStringProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::StringProperty");
  }
  return newObject;
};
tulip.StringProperty.inheritsFrom(tulip.PropertyInterface);
tulip.StringProperty.prototype.getNodeDefaultValue = function tulip_StringProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _StringProperty_getNodeDefaultValue(this.cppPointer);
};
tulip.StringProperty.prototype.getNodeValue = function tulip_StringProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  return _StringProperty_getNodeValue(this.cppPointer, node.id);
};
tulip.StringProperty.prototype.setNodeValue = function tulip_StringProperty_prototype_setNodeValue(node, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "string"]);
  _StringProperty_setNodeValue(this.cppPointer, node.id, val);
};
tulip.StringProperty.prototype.getEdgeDefaultValue = function tulip_StringProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  return _StringProperty_getEdgeDefaultValue(this.cppPointer);
};
tulip.StringProperty.prototype.getEdgeValue = function tulip_StringProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  return _StringProperty_getEdgeValue(this.cppPointer, edge.id);
};
tulip.StringProperty.prototype.setEdgeValue = function tulip_StringProperty_prototype_setEdgeValue(edge, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "string"]);
  _StringProperty_setEdgeValue(this.cppPointer, edge.id, val);
};
tulip.StringProperty.prototype.setAllNodeValue = function tulip_StringProperty_prototype_setAllNodeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  _StringProperty_setAllNodeValue(this.cppPointer, val);
};
tulip.StringProperty.prototype.setAllEdgeValue = function tulip_StringProperty_prototype_setAllEdgeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  _StringProperty_setAllEdgeValue(this.cppPointer, val);
};

// ==================================================================================================================

var _createStringVectorProperty = Module.cwrap('createStringVectorProperty', 'number', ['number', 'string']);
var _StringVectorProperty_setAllNodeValue = Module.cwrap('StringVectorProperty_setAllNodeValue', null, ['number', 'number', 'number', 'number']);
var _StringVectorProperty_setNodeValue = Module.cwrap('StringVectorProperty_setNodeValue', null, ['number', 'number', 'number', 'number', 'number']);
var _StringVectorProperty_setAllEdgeValue = Module.cwrap('StringVectorProperty_setAllEdgeValue', null, ['number', 'number', 'number', 'number']);
var _StringVectorProperty_setEdgeValue = Module.cwrap('StringVectorProperty_setEdgeValue', null, ['number', 'number', 'number', 'number', 'number']);

var _StringVectorProperty_getNodeDefaultVectorSize = Module.cwrap('StringVectorProperty_getNodeDefaultVectorSize', 'number', ['number']);
var _StringVectorProperty_getNodeDefaultStringsLengths = Module.cwrap('StringVectorProperty_getNodeDefaultStringsLengths', 'number', ['number', 'number']);
var _StringVectorProperty_getNodeDefaultValue = Module.cwrap('StringVectorProperty_getNodeDefaultValue', null, ['number', 'number']);

var _StringVectorProperty_getNodeVectorSize = Module.cwrap('StringVectorProperty_getNodeVectorSize', 'number', ['number', 'number']);
var _StringVectorProperty_getNodeStringsLengths = Module.cwrap('StringVectorProperty_getNodeStringsLengths', 'number', ['number', 'number', 'number']);
var _StringVectorProperty_getNodeValue = Module.cwrap('StringVectorProperty_getNodeValue', null, ['number', 'number', 'number']);

var _StringVectorProperty_getEdgeVectorSize = Module.cwrap('StringVectorProperty_getEdgeVectorSize', 'number', ['number', 'number']);
var _StringVectorProperty_getEdgeStringsLengths = Module.cwrap('StringVectorProperty_getEdgeStringsLengths', 'number', ['number', 'number', 'number']);
var _StringVectorProperty_getEdgeValue = Module.cwrap('StringVectorProperty_getEdgeValue', null, ['number', 'number', 'number']);

var _StringVectorProperty_getEdgeDefaultVectorSize = Module.cwrap('StringVectorProperty_getEdgeDefaultVectorSize', 'number', ['number']);
var _StringVectorProperty_getEdgeDefaultStringsLengths = Module.cwrap('StringVectorProperty_getEdgeDefaultStringsLengths', 'number', ['number', 'number']);
var _StringVectorProperty_getEdgeDefaultValue = Module.cwrap('StringVectorProperty_getEdgeDefaultValue', null, ['number', 'number']);

tulip.StringVectorProperty = function tulip_StringVectorProperty() {
  var newObject = createObject(tulip.StringVectorProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createStringVectorProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::StringVectorProperty");
  }
  return newObject;
};
tulip.StringVectorProperty.inheritsFrom(tulip.PropertyInterface);

tulip.StringVectorProperty.prototype.getNodeDefaultValue = function tulip_StringVectorProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var nbStrings = _StringVectorProperty_getNodeDefaultVectorSize(this.cppPointer);
  var uintArray = _allocArrayInEmHeap(Uint32Array, nbStrings);
  var nbBytes = _StringVectorProperty_getNodeDefaultStringsLengths(this.cppPointer, uintArray.byteOffset);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  _StringVectorProperty_getNodeDefaultValue(this.cppPointer, ucharArray.byteOffset);
  var ret = _bytesTypedArrayToStringArray(ucharArray, uintArray, nbStrings);
  _freeArrayInEmHeap(uintArray);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.StringVectorProperty.prototype.getNodeValue = function tulip_StringVectorProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var nbStrings = _StringVectorProperty_getNodeVectorSize(this.cppPointer, node.id);
  var uintArray = _allocArrayInEmHeap(Uint32Array, nbStrings);
  var nbBytes = _StringVectorProperty_getNodeStringsLengths(this.cppPointer, node.id, uintArray.byteOffset);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  _StringVectorProperty_getNodeValue(this.cppPointer, node.id, ucharArray.byteOffset);
  var ret = _bytesTypedArrayToStringArray(ucharArray, uintArray, nbStrings);
  _freeArrayInEmHeap(uintArray);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.StringVectorProperty.prototype.setNodeValue = function tulip_StringVectorProperty_prototype_setNodeValue(node, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "array"]);
  checkArrayOfType(val, "string", 1);
  var data = _stringArrayToBytesAndOffsetsTypedArray(val);
  _StringVectorProperty_setNodeValue(this.cppPointer, node.id, data.bytesArray.byteOffset, data.offsetsArray.byteOffset, val.length);
  _freeArrayInEmHeap(data.bytesArray);
  _freeArrayInEmHeap(data.offsetsArray);
};
tulip.StringVectorProperty.prototype.getEdgeDefaultValue = function tulip_StringVectorProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var nbStrings = _StringVectorProperty_getEdgeDefaultVectorSize(this.cppPointer);
  var uintArray = _allocArrayInEmHeap(Uint32Array, nbStrings);
  var nbBytes = _StringVectorProperty_getEdgeDefaultStringsLengths(this.cppPointer, uintArray.byteOffset);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  _StringVectorProperty_getEdgeDefaultValue(this.cppPointer, ucharArray.byteOffset);
  var ret = _bytesTypedArrayToStringArray(ucharArray, uintArray, nbStrings);
  _freeArrayInEmHeap(uintArray);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.StringVectorProperty.prototype.getEdgeValue = function tulip_StringVectorProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  var nbStrings = _StringVectorProperty_getEdgeVectorSize(this.cppPointer, edge.id);
  var uintArray = _allocArrayInEmHeap(Uint32Array, nbStrings);
  var nbBytes = _StringVectorProperty_getEdgeStringsLengths(this.cppPointer, edge.id, uintArray.byteOffset);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  _StringVectorProperty_getEdgeValue(this.cppPointer, edge.id, ucharArray.byteOffset);
  var ret = _bytesTypedArrayToStringArray(ucharArray, uintArray, nbStrings);
  _freeArrayInEmHeap(uintArray);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.StringVectorProperty.prototype.setEdgeValue = function tulip_StringVectorProperty_prototype_setEdgeValue(edge, val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "array"]);
  checkArrayOfType(val, "string", 1);
  var data = _stringArrayToBytesAndOffsetsTypedArray(val);
  _StringVectorProperty_setEdgeValue(this.cppPointer, edge.id, data.bytesArray.byteOffset, data.offsetsArray.byteOffset, val.length);
  _freeArrayInEmHeap(data.bytesArray);
  _freeArrayInEmHeap(data.offsetsArray);
};
tulip.StringVectorProperty.prototype.setAllNodeValue = function tulip_StringVectorProperty_prototype_setAllNodeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"]);
  checkArrayOfType(val, "string", 0);
  var data = _stringArrayToBytesAndOffsetsTypedArray(val);
  _StringVectorProperty_setAllNodeValue(this.cppPointer, data.bytesArray.byteOffset, data.offsetsArray.byteOffset, val.length);
  _freeArrayInEmHeap(data.bytesArray);
  _freeArrayInEmHeap(data.offsetsArray);
};
tulip.StringVectorProperty.prototype.setAllEdgeValue = function tulip_StringVectorProperty_prototype_setAllEdgeValue(val) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"]);
  checkArrayOfType(val, "string", 0);
  var data = _stringArrayToBytesAndOffsetsTypedArray(val);
  _StringVectorProperty_setAllEdgeValue(this.cppPointer, data.bytesArray.byteOffset, data.offsetsArray.byteOffset, val.length);
  _freeArrayInEmHeap(data.bytesArray);
  _freeArrayInEmHeap(data.offsetsArray);
};

// ==================================================================================================================

tulip.Color = function tulip_Color(r, g, b, a) {
  checkArgumentsTypes(arguments, ["number", "number", "number", "number"]);
  var newObject = createObject(tulip.Color, this);
  newObject.r = r;
  newObject.g = g;
  newObject.b = b;
  if (arguments.length < 4) {
    newObject.a = 255;
  } else {
    newObject.a = a;
  }
  return newObject;
};

// ==================================================================================================================

var _createColorProperty = Module.cwrap('createColorProperty', 'number', ['number', 'string']);
var _ColorProperty_setAllNodeValue = Module.cwrap('ColorProperty_setAllNodeValue', null, ['number', 'number', 'number', 'number', 'number']);
var _ColorProperty_setNodeValue = Module.cwrap('ColorProperty_setNodeValue', null, ['number', 'number', 'number', 'number', 'number', 'number']);
var _ColorProperty_getNodeDefaultValue = Module.cwrap('ColorProperty_getNodeDefaultValue', null, ['number', 'number']);
var _ColorProperty_getNodeValue = Module.cwrap('ColorProperty_getNodeValue', null, ['number', 'number', 'number']);
var _ColorProperty_setAllEdgeValue = Module.cwrap('ColorProperty_setAllEdgeValue', null, ['number', 'number', 'number', 'number', 'number']);
var _ColorProperty_setEdgeValue = Module.cwrap('ColorProperty_setEdgeValue', null, ['number', 'number', 'number', 'number', 'number', 'number']);
var _ColorProperty_getEdgeDefaultValue = Module.cwrap('ColorProperty_getEdgeDefaultValue', null, ['number', 'number']);
var _ColorProperty_getEdgeValue = Module.cwrap('ColorProperty_getEdgeValue', null, ['number', 'number', 'number']);

tulip.ColorProperty = function tulip_ColorProperty() {
  var newObject = createObject(tulip.ColorProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createColorProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::ColorProperty");
  }
  return newObject;
};
tulip.ColorProperty.inheritsFrom(tulip.PropertyInterface);
tulip.ColorProperty.prototype.getNodeDefaultValue = function tulip_ColorProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, 4);
  _ColorProperty_getNodeDefaultValue(this.cppPointer, ucharArray.byteOffset);
  var ret = tulip.Color(ucharArray[0], ucharArray[1], ucharArray[2], ucharArray[3]);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.ColorProperty.prototype.getNodeValue = function tulip_ColorProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node], 1);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, 4);
  _ColorProperty_getNodeValue(this.cppPointer, node.id, ucharArray.byteOffset);
  var ret = tulip.Color(ucharArray[0], ucharArray[1], ucharArray[2], ucharArray[3]);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.ColorProperty.prototype.setNodeValue = function tulip_ColorProperty_prototype_setNodeValue(node, color) {
  checkArgumentsTypes(arguments, [tulip.Node, tulip.Color], 2);
  _ColorProperty_setNodeValue(this.cppPointer, node.id, color.r, color.g, color.b, color.a);
};
tulip.ColorProperty.prototype.getEdgeDefaultValue = function tulip_ColorProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, 4);
  _ColorProperty_getEdgeDefaultValue(this.cppPointer, ucharArray.byteOffset);
  var ret = tulip.Color(ucharArray[0], ucharArray[1], ucharArray[2], ucharArray[3]);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.ColorProperty.prototype.getEdgeValue = function tulip_ColorProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge], 1);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, 4);
  _ColorProperty_getEdgeValue(this.cppPointer, edge.id, ucharArray.byteOffset);
  var ret = tulip.Color(ucharArray[0], ucharArray[1], ucharArray[2], ucharArray[3]);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.ColorProperty.prototype.setEdgeValue = function tulip_ColorProperty_prototype_setEdgeValue(edge, color) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, tulip.Color], 2);
  _ColorProperty_setEdgeValue(this.cppPointer, edge.id, color.r, color.g, color.b, color.a);
};
tulip.ColorProperty.prototype.setAllNodeValue = function tulip_ColorProperty_prototype_setAllNodeValue(color) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Color], 1);
  _ColorProperty_setAllNodeValue(this.cppPointer, color.r, color.g, color.b, color.a);
};
tulip.ColorProperty.prototype.setAllEdgeValue = function tulip_ColorProperty_prototype_setAllEdgeValue(color) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Color], 1);
  _ColorProperty_setAllEdgeValue(this.cppPointer, color.r, color.g, color.b, color.a);
};

// ==================================================================================================================

tulip.Vec3f = function tulip_Vec3f(x, y, z) {
  checkArgumentsTypes(arguments, ["number", "number", "number"])
  var newObject = createObject(tulip.Vec3f, this);
  newObject.x = 0;
  newObject.y = 0;
  newObject.z = 0;
  if (arguments.length == 1) {
    newObject.x = newObject.y = newObject.z = x;
  } else if (arguments.length == 2) {
    newObject.x = x;
    newObject.y = y;
  } else if (arguments.length == 3) {
    newObject.x = x;
    newObject.y = y;
    newObject.z = z;
  }
  return newObject;
};
tulip.Vec3f.prototype.add = function tulip_Vec3f_prototype_add() {
  var types = [];
  for (var i = 0 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 1);
  for (var i = 0 ; i < arguments.length ; ++i) {
    if (typeOf(arguments[i]) == 'number') {
      this.x += arguments[i];
      this.y += arguments[i];
      this.z += arguments[i];
    } else {
      this.x += arguments[i].x;
      this.y += arguments[i].y;
      this.z += arguments[i].z;
    }
  }
  return this;
};
tulip.Vec3f.add = function tulip_Vec3f_add() {
  var types = [];
  types.push(tulip.Vec3f);
  for (var i = 1 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 2);
  var p = arguments[0];
  for (var i = 1 ; i < arguments.length ; ++i) {
    if (typeOf(arguments[i]) == 'number') {
      p.x += arguments[i];
      p.y += arguments[i];
      p.z += arguments[i];
    } else {
      p.x += arguments[i].x;
      p.y += arguments[i].y;
      p.z += arguments[i].z;
    }
  }
  return p;
};
tulip.Vec3f.prototype.sub = function tulip_Vec3f_prototype_sub() {
  var types = [];
  for (var i = 0 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 1);
  for (var i = 0 ; i < arguments.length ; ++i) {
    this.x -= arguments[i].x;
    this.y -= arguments[i].y;
    this.z -= arguments[i].z;
  }
  return this;
};
tulip.Vec3f.sub = function tulip_Vec3f_sub() {
  var types = [];
  types.push(tulip.Vec3f);
  for (var i = 1 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 2);
  var p = arguments[0];
  for (var i = 1 ; i < arguments.length ; ++i) {
    if (typeOf(arguments[i]) == 'number') {
      p.x -= arguments[i];
      p.y -= arguments[i];
      p.z -= arguments[i];
    } else {
      p.x -= arguments[i].x;
      p.y -= arguments[i].y;
      p.z -= arguments[i].z;
    }
  }
  return p;
};
tulip.Vec3f.prototype.mul = function tulip_Vec3f_prototype_mul() {
  var types = [];
  for (var i = 0 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 1);
  for (var i = 0 ; i < arguments.length ; ++i) {
    if (typeOf(arguments[i]) == 'number') {
      this.x *= arguments[i];
      this.y *= arguments[i];
      this.z *= arguments[i];
    } else {
      this.x *= arguments[i].x;
      this.y *= arguments[i].y;
      this.z *= arguments[i].z;
    }
  }
  return this;
};
tulip.Vec3f.mul = function tulip_Vec3f_mul() {
  var types = [];
  types.push(tulip.Vec3f);
  for (var i = 1 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 2);
  var p = arguments[0];
  for (var i = 1 ; i < arguments.length ; ++i) {
    if (typeOf(arguments[i]) == 'number') {
      p.x *= arguments[i];
      p.y *= arguments[i];
      p.z *= arguments[i];
    } else {
      p.x *= arguments[i].x;
      p.y *= arguments[i].y;
      p.z *= arguments[i].z;
    }
  }
  return p;
};
tulip.Vec3f.prototype.div = function tulip_Vec3f_prototype_div() {
  var types = [];
  for (var i = 0 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 1);
  for (var i = 0 ; i < arguments.length ; ++i) {
    if (typeOf(arguments[i]) == 'number') {
      this.x /= arguments[i];
      this.y /= arguments[i];
      this.z /= arguments[i];
    } else {
      this.x /= arguments[i].x;
      this.y /= arguments[i].y;
      this.z /= arguments[i].z;
    }
  }
  return this;
};
tulip.Vec3f.div = function tulip_Vec3f_div() {
  var types = [];
  types.push(tulip.Vec3f);
  for (var i = 1 ; i < arguments.length ; ++i) {
    types.push(['number', tulip.Vec3f]);
  }
  checkArgumentsTypes(arguments, types, 2);
  var p = arguments[0];
  for (var i = 1 ; i < arguments.length ; ++i) {
    p.x *= arguments[i].x;
    p.y *= arguments[i].y;
    p.z *= arguments[i].z;
  }
  return p;
};
tulip.Vec3f.prototype.norm = function tulip_Vec3f_prototype_norm() {
  return Math.sqrt(this.x*this.x+this.y*this.y+this.z*this.z);
};
tulip.Vec3f.prototype.normalize = function tulip_Vec3f_prototype_normalize() {
  var n = this.norm();
  if (n != 0) {
    this.div(n);
  }
};
tulip.Vec3f.prototype.dist = function tulip_Vec3f_prototype_dist(c) {
  checkArgumentsTypes(arguments, [tulip.Vec3f]);
  return tulip.Vec3f.dist(this, c);
};
tulip.Vec3f.dist = function tulip_Vec3f_dist(c1, c2) {
  checkArgumentsTypes(arguments, [tulip.Vec3f, tulip.Vec3f]);
  return tulip.Vec3f.sub(c1, c2).norm();
};
tulip.Vec3f.min = function tulip_Vec3f_min() {
  var types = [];
  for (var i = 0 ; i < arguments.length ; ++i) {
    types.push(tulip.Vec3f);
  }
  checkArgumentsTypes(arguments, types, 2);
  var ret = arguments[0];
  for (var i = 1 ; i < arguments.length ; ++i) {
    ret.x = Math.min(ret.x, arguments[i].x);
    ret.y = Math.min(ret.y, arguments[i].y);
    ret.z = Math.min(ret.z, arguments[i].z);
  }
  return ret;
};
tulip.Vec3f.max = function tulip_Vec3f_max() {
  var types = [];
  for (var i = 0 ; i < arguments.length ; ++i) {
    types.push(tulip.Vec3f);
  }
  checkArgumentsTypes(arguments, types, 2);
  var ret = arguments[0];
  for (var i = 1 ; i < arguments.length ; ++i) {
    ret.x = Math.max(ret.x, arguments[i].x);
    ret.y = Math.max(ret.y, arguments[i].y);
    ret.z = Math.max(ret.z, arguments[i].z);
  }
  return ret;
};
tulip.Vec3f.lt = function tulip_Vec3f_lt(v1, v2) {
  checkArgumentsTypes(arguments, [tulip.Vec3f, tulip.Vec3f], 2);
  return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z;
};
tulip.Vec3f.prototype.lt = function tulip_Vec3f_prototype_lt(v) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 1);
  return tulip.Vec3f.lt(this, v);
};
tulip.Vec3f.leq = function tulip_Vec3f_leq(v1, v2) {
  checkArgumentsTypes(arguments, [tulip.Vec3f, tulip.Vec3f], 2);
  return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z;
};
tulip.Vec3f.prototype.leq = function tulip_Vec3f_prototype_leq(v) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 1);
  return tulip.Vec3f.leq(this, v);
};
tulip.Vec3f.gt = function tulip_Vec3f_gt(v1, v2) {
  checkArgumentsTypes(arguments, [tulip.Vec3f, tulip.Vec3f], 2);
  return v1.x > v2.x && v1.y > v2.y && v1.z > v2.z;
};
tulip.Vec3f.prototype.gt = function tulip_Vec3f_prototype_gt(v) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 1);
  return tulip.Vec3f.gt(this, v);
};
tulip.Vec3f.geq = function tulip_Vec3f_geq(v1, v2) {
  checkArgumentsTypes(arguments, [tulip.Vec3f, tulip.Vec3f], 2);
  return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z;
};
tulip.Vec3f.prototype.geq = function tulip_Vec3f_prototype_geq(v) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 1);
  return tulip.Vec3f.geq(this, v);
};
tulip.Vec3f.dot = function tulip_Vec3f_dot(v1, v2) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 2);
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
};
tulip.Vec3f.prototype.dot = function tulip_Vec3f_prototype_dot(v) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 1);
  return tulip.Vec3f.dot(this, v);
};
tulip.Vec3f.cross = function tulip_Vec3f_cross(v1, v2) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 2);
  var x = v1.y * v2.z - v1.z * v2.y;
  var y = v1.z * v2.x - v1.x * v2.z;
  var z = v1.x * v2.y - v1.y * v2.x;
  return tulip.Vec3f(x, y, z);
};
tulip.Vec3f.prototype.cross = function tulip_Vec3f_prototype_cross(v) {
  checkArgumentsTypes(arguments, [tulip.Vec3f], 1);
  return tulip.Vec3f.cross(this, v);
};
tulip.Vec3f.prototype.getX = function tulip_Vec3f_prototype_getX() {
  return this.x;
};
tulip.Vec3f.prototype.getY = function tulip_Vec3f_prototype_getY() {
  return this.y;
};
tulip.Vec3f.prototype.getZ = function tulip_Vec3f_prototype_getZ() {
  return this.z;
};
tulip.Vec3f.prototype.setX = function tulip_Vec3f_prototype_setX(x) {
  checkArgumentsTypes(arguments, ["number"], 1);
  this.x = x;
};
tulip.Vec3f.prototype.setY = function tulip_Vec3f_prototype_setY(y) {
  checkArgumentsTypes(arguments, ["number"], 1);
  this.y = y;
};
tulip.Vec3f.prototype.setZ = function tulip_Vec3f_prototype_setZ(z) {
  checkArgumentsTypes(arguments, ["number"], 1);
  this.z = z;
};
tulip.Vec3f.prototype.getWidth = function tulip_Vec3f_prototype_getWidth() {
  return this.x;
};
tulip.Vec3f.prototype.getHeight = function tulip_Vec3f_prototype_getHeight() {
  return this.y;
};
tulip.Vec3f.prototype.getDepth = function tulip_Vec3f_prototype_getDepth() {
  return this.z;
};
tulip.Vec3f.prototype.setWidth = function tulip_Vec3f_prototype_setWidth(w) {
  checkArgumentsTypes(arguments, ["number"], 1);
  this.x = w;
};
tulip.Vec3f.prototype.setHeight = function tulip_Vec3f_prototype_setHeight(h) {
  checkArgumentsTypes(arguments, ["number"], 1);
  this.y = h;
};
tulip.Vec3f.prototype.setDepth = function tulip_Vec3f_prototype_setDepth(d) {
  checkArgumentsTypes(arguments, ["number"], 1);
  this.z = d;
};


tulip.Coord = tulip.Vec3f;
tulip.Size = tulip.Vec3f;

// ==================================================================================================================

var _createLayoutProperty = Module.cwrap('createLayoutProperty', 'number', ['number', 'string']);
var _LayoutProperty_setNodeValue = Module.cwrap('LayoutProperty_setNodeValue', null, ['number', 'number', 'number', 'number', 'number']);
var _LayoutProperty_setAllNodeValue = Module.cwrap('LayoutProperty_setAllNodeValue', null, ['number', 'number', 'number', 'number']);
var _LayoutProperty_getNodeDefaultValue = Module.cwrap('LayoutProperty_getNodeDefaultValue', null, ['number', 'number']);
var _LayoutProperty_getNodeValue = Module.cwrap('LayoutProperty_getNodeValue', null, ['number', 'number', 'number']);
var _LayoutProperty_getEdgeDefaultNumberOfBends = Module.cwrap('LayoutProperty_getEdgeDefaultNumberOfBends', 'number', ['number']);
var _LayoutProperty_getEdgeDefaultValue = Module.cwrap('LayoutProperty_getEdgeDefaultValue', null, ['number', 'number']);
var _LayoutProperty_getEdgeNumberOfBends = Module.cwrap('LayoutProperty_getEdgeNumberOfBends', 'number', ['number', 'number']);
var _LayoutProperty_getEdgeValue = Module.cwrap('LayoutProperty_getEdgeValue', null, ['number', 'number', 'number']);
var _LayoutProperty_setEdgeValue = Module.cwrap('LayoutProperty_setAllEdgeValue', null, ['number', 'number', 'number', 'number']);
var _LayoutProperty_setAllEdgeValue = Module.cwrap('LayoutProperty_setAllEdgeValue', null, ['number', 'number', 'number']);

tulip.LayoutProperty = function tulip_LayoutProperty() {
  var newObject = createObject(tulip.LayoutProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createLayoutProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::LayoutProperty");
  }
  return newObject;
};
tulip.LayoutProperty.inheritsFrom(tulip.PropertyInterface);
tulip.LayoutProperty.prototype.getNodeDefaultValue = function tulip_LayoutProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _LayoutProperty_getNodeDefaultValue(this.cppPointer, floatArray.byteOffset);
  var ret = tulip.Coord(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.LayoutProperty.prototype.getNodeValue = function tulip_LayoutProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node], 1);
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _LayoutProperty_getNodeValue(this.cppPointer, node.id, floatArray.byteOffset);
  var ret = tulip.Coord(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.LayoutProperty.prototype.setNodeValue = function tulip_LayoutProperty_prototype_setNodeValue(node, coord) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, tulip.Coord], 2);
  _LayoutProperty_setNodeValue(this.cppPointer, node.id, coord.x, coord.y, coord.z);
};
tulip.LayoutProperty.prototype.setAllNodeValue = function tulip_LayoutProperty_prototype_setAllNodeValue(coord) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Coord], 1);
  _LayoutProperty_setAllNodeValue(this.cppPointer, coord.x, coord.y, coord.z);
};
tulip.LayoutProperty.prototype.getEdgeDefaultValue = function tulip_LayoutProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var nbBends = _LayoutProperty_getEdgeDefaultNumberOfBends(this.cppPointer);
  var floatArray = _allocArrayInEmHeap(Float32Array, nbBends*3);
  _LayoutProperty_getEdgeDefaultValue(this.cppPointer, floatArray.byteOffset);
  var ret = [];
  for (var i = 0 ; i < nbBends ; ++i) {
    ret.push(tulip.Coord(floatArray[3*i], floatArray[3*i+1], floatArray[3*i+2]));
  }
  _freeArrayInEmHeap(floatArray);
  return ret;
};

tulip.LayoutProperty.prototype.getEdgeValue = function tulip_LayoutProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge], 1);
  var nbBends = _LayoutProperty_getEdgeNumberOfBends(this.cppPointer, edge.id);
  var floatArray = _allocArrayInEmHeap(Float32Array, nbBends*3);
  _LayoutProperty_getEdgeValue(this.cppPointer, edge.id, floatArray.byteOffset);
  var ret = [];
  for (var i = 0 ; i < nbBends ; ++i) {
    ret.push(tulip.Coord(floatArray[3*i], floatArray[3*i+1], floatArray[3*i+2]));
  }
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.LayoutProperty.prototype.setEdgeValue = function tulip_LayoutProperty_prototype_setEdgeValue(edge, bends) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "array"]);
  checkArrayOfType(bends, tulip.Coord, 1);
  var floatArray = _allocArrayInEmHeap(Float32Array, bends.length*3);
  for (var i = 0 ; i < bends.length ; ++i) {
    floatArray[3*i] = bends[i].x;
    floatArray[3*i+1] = bends[i].y;
    floatArray[3*i+2] = bends[i].z;
  }
  _LayoutProperty_setEdgeValue(this.cppPointer, edge.id, floatArray.byteOffset, bends.length);
  _freeArrayInEmHeap(floatArray);
};
tulip.LayoutProperty.prototype.setAllEdgeValue = function tulip_LayoutProperty_prototype_setAllEdgeValue(bends) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"]);
  checkArrayOfType(bends, tulip.Coord, 0);
  var floatArray = _allocArrayInEmHeap(Float32Array, bends.length*3);
  for (var i = 0 ; i < bends.length ; ++i) {
    floatArray[3*i] = bends[i].x;
    floatArray[3*i+1] = bends[i].y;
    floatArray[3*i+2] = bends[i].z;
  }
  _LayoutProperty_setAllEdgeValue(this.cppPointer, floatArray.byteOffset, bends.length);
  _freeArrayInEmHeap(floatArray);
};

// ==================================================================================================================

var _createSizeProperty = Module.cwrap('createSizeProperty', 'number', ['number', 'string']);
var _SizeProperty_setNodeValue = Module.cwrap('SizeProperty_setNodeValue', null, ['number', 'number', 'number', 'number', 'number']);
var _SizeProperty_setAllNodeValue = Module.cwrap('SizeProperty_setAllNodeValue', null, ['number', 'number', 'number', 'number']);
var _SizeProperty_getNodeDefaultValue = Module.cwrap('SizeProperty_getNodeDefaultValue', null, ['number', 'number']);
var _SizeProperty_getNodeValue = Module.cwrap('SizeProperty_getNodeValue', null, ['number', 'number', 'number']);
var _SizeProperty_getEdgeDefaultValue = Module.cwrap('SizeProperty_getEdgeDefaultValue', null, ['number', 'number']);
var _SizeProperty_getEdgeValue = Module.cwrap('SizeProperty_getEdgeValue', null, ['number', 'number', 'number']);
var _SizeProperty_setEdgeValue = Module.cwrap('SizeProperty_setAllEdgeValue', null, ['number', 'number', 'number', 'number', 'number']);
var _SizeProperty_setAllEdgeValue = Module.cwrap('SizeProperty_setAllEdgeValue', null, ['number', 'number', 'number', 'number']);
var _SizeProperty_scale = Module.cwrap('SizeProperty_scale', null, ['number', 'number', 'number', 'number', 'number']);
var _SizeProperty_getMin = Module.cwrap('SizeProperty_getMin', null, ['number', 'number', 'number']);
var _SizeProperty_getMax = Module.cwrap('SizeProperty_getMax', null, ['number', 'number', 'number']);

tulip.SizeProperty = function() {
  var newObject = createObject(tulip.SizeProperty, this);
  if (arguments.callee.caller == null || arguments.callee.caller.name != "createObject") {
    var cppPointer = 0;
    var graphManaged = false;
    if (arguments.length == 1 && typeOf(arguments[0]) == "number") {
      cppPointer = arguments[0];
      graphManaged = true;
    } else {
      checkArgumentsTypes(arguments, [tulip.Graph, "string"], 1);
      var propName = "";
      if (arguments.length > 1) propName = arguments[1];
      cppPointer = _createSizeProperty(arguments[0].cppPointer, propName);
    }
    tulip.PropertyInterface.call(newObject, cppPointer, graphManaged);
    newObject.setWrappedTypename("tlp::SizeProperty");
  }
  return newObject;
};
tulip.SizeProperty.inheritsFrom(tulip.PropertyInterface);
tulip.SizeProperty.prototype.getNodeDefaultValue = function tulip_SizeProperty_prototype_getNodeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _SizeProperty_getNodeDefaultValue(this.cppPointer, floatArray.byteOffset);
  var ret = tulip.Size(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.SizeProperty.prototype.getNodeValue = function tulip_SizeProperty_prototype_getNodeValue(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node], 1);
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _SizeProperty_getNodeValue(this.cppPointer, node.id, floatArray.byteOffset);
  var ret = tulip.Size(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.SizeProperty.prototype.setNodeValue = function tulip_SizeProperty_prototype_setNodeValue(node, size) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, tulip.Size]);
  _SizeProperty_setNodeValue(this.cppPointer, node.id, size.getWidth(), size.getHeight(), size.getDepth());
};
tulip.SizeProperty.prototype.setAllNodeValue = function tulip_SizeProperty_prototype_setAllNodeValue(size) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Size]);
  _SizeProperty_setAllNodeValue(this.cppPointer, size.getWidth(), size.getHeight(), size.getDepth());
};
tulip.SizeProperty.prototype.getEdgeDefaultValue = function tulip_SizeProperty_prototype_getEdgeDefaultValue() {
  checkWrappedCppPointer(this.cppPointer);
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _SizeProperty_getEdgeDefaultValue(this.cppPointer, floatArray.byteOffset);
  var ret = tulip.Size(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.SizeProperty.prototype.getEdgeValue = function tulip_SizeProperty_prototype_getEdgeValue(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge], 1);
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _SizeProperty_getEdgeValue(this.cppPointer, edge.id, floatArray.byteOffset);
  var ret = tulip.Size(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.SizeProperty.prototype.setEdgeValue = function tulip_SizeProperty_prototype_setEdgeValue(edge, size) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, tulip.Size]);
  _SizeProperty_setEdgeValue(this.cppPointer, edge.id, size.getWidth(), size.getHeight(), size.getDepth());
};
tulip.SizeProperty.prototype.setAllEdgeValue = function tulip_SizeProperty_prototype_setAllEdgeValue(size) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Size]);
  _SizeProperty_setAllEdgeValue(this.cppPointer, size.getWidth(), size.getHeight(), size.getDepth());
};
tulip.SizeProperty.prototype.scale = function tulip_SizeProperty_prototype_scale(sizeFactor, subgraph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Size, tulip.Graph], 1);
  var sgPointer = 0;
  if (arguments.length > 1) sgPointer = subgraph.cppPointer;
  _SizeProperty_scale(this.cppPointer, sizeFactor.getWidth(), sizeFactor.getHeight(), sizeFactor.getDepth(), sgPointer);
};
tulip.SizeProperty.prototype.getMin = function tulip_SizeProperty_prototype_getMin(subgraph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Graph]);
  var sgPointer= 0;
  if (arguments.length == 1) {
    sgPointer = subgraph.cppPointer;
  }
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _SizeProperty_getMin(this.cppPointer, sgPointer, floatArray.byteOffset);
  var ret = tulip.Size(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};
tulip.SizeProperty.prototype.getMax = function tulip_SizeProperty_prototype_getMax(subgraph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Graph]);
  var sgPointer= 0;
  if (arguments.length == 1) {
    sgPointer = subgraph.cppPointer;
  }
  var floatArray = _allocArrayInEmHeap(Float32Array, 3);
  _SizeProperty_getMax(this.cppPointer, sgPointer, floatArray.byteOffset);
  var ret = tulip.Size(floatArray[0], floatArray[1], floatArray[2]);
  _freeArrayInEmHeap(floatArray);
  return ret;
};

// ==================================================================================================================

tulip.BoundingBox = function tulip_BoundingBox() {
  var newObject = createObject(tulip.BoundingBox, this);
  if (arguments.length == 0) {
    newObject.min = tulip.Coord(1,1,1);
    newObject.max = tulip.Coord(-1,-1,-1);
  } else {
    checkArgumentsTypes(arguments, [tulip.Coord, tulip.Coord], 2);
    newObject.min = arguments[0];
    newObject.max = arguments[1];
  }
  return newObject;
};
tulip.BoundingBox.prototype.isValid = function tulip_BoundingBox_prototype_isValid() {
  return this.min.x <= this.max.x && this.min.y <= this.max.y && this.min.z <= this.min.z;
};
tulip.BoundingBox.prototype.center = function tulip_BoundingBox_prototype_center() {
  return tulip.Coord.add(this.min, this.max).div(2);
};
tulip.BoundingBox.prototype.width = function tulip_BoundingBox_prototype_width() {
  return this.max.x - this.min.x;
};
tulip.BoundingBox.prototype.height = function tulip_BoundingBox_prototype_height() {
  return this.max.y - this.min.y;
};
tulip.BoundingBox.prototype.depth = function tulip_BoundingBox_prototype_depth() {
  return this.max.z - this.min.z;
};
tulip.BoundingBox.prototype.expand = function tulip_BoundingBox_prototype_depth(coord) {
  checkArgumentsTypes(arguments, [tulip.Coord], 1);
  if (!this.isValid()) {
    this.min = arguments[0];
    this.max = arguments[0];
  }
  this.min = tulip.Coord.min.apply(null, [this.min].concat(Array.slice(arguments)));
  this.max = tulip.Coord.max.apply(null, [this.max].concat(Array.slice(arguments)));
};
tulip.BoundingBox.prototype.translate = function tulip_BoundingBox_prototype_translate(vec) {
  checkArgumentsTypes(arguments, [tulip.Coord], 1);
  this.min.add(vec);
  this.max.add(vec);
};
tulip.BoundingBox.prototype.scale = function tulip_BoundingBox_prototype_scale(vec) {
  checkArgumentsTypes(arguments, [tulip.Coord], 1);
  this.min.mul(vec);
  this.max.mul(vec);
};
tulip.BoundingBox.prototype.contains = function tulip_BoundingBox_prototype_contains(obj) {
  checkArgumentsTypes(arguments, [[tulip.Coord, tulip.BoundingBox]], 1);
  if (obj instanceof tulip.Coord) {
    if (this.isValid()) {
      return coord.x >= this.min.x && coord.y >= this.min.y && coord.z >= this.min.z && coord.x <= this.max.x && coord.y <= this.max.y && coord.z <= this.max.z;
    } else {
      return false;
    }
  } else {
    if (this.isValid() && obj.isValid()) {
      return this.contains(obj.min) && this.contains(obj.max);
    } else {
      return false;
    }
  }
};
tulip.BoundingBox.prototype.intersect = function tulip_BoundingBox_prototype_intersect(bb) {
  checkArgumentsTypes(arguments, [tulip.BoundingBox], 1);
  if (!this.isValid() || !bb.isValid()) {
    return false;
  }
  if (this.max.x < bb.min.x) return false;
  if (bb.max.x < this.min.x) return false;
  if (this.max.y < bb.min.y) return false;
  if (bb.max.y < this.min.y) return false;
  if (this.max.z < bb.min.z) return false;
  if (bb.max.z < this.min.z) return false;
  return true;
};


// ==================================================================================================================
/**
* This is the description for the tulip.node class.
*
* @class Node
* @constructor
*/
tulip.Node = function tulip_Node(id) {
  checkArgumentsTypes(arguments, ["number"]);
  var newObject = createObject(tulip.Node, this);
  if (arguments.length == 0) {
    newObject.id = UINT_MAX;
  } else {
    newObject.id = id;
  }
  return newObject;
};
tulip.Node.prototype.isValid = function() {
  return this.id != UINT_MAX;
};

// ==================================================================================================================
/**
* This is the description for the tulip.edge class.
*
* @class Edge
* @constructor
*/
tulip.Edge = function tulip_Edge(id) {
  checkArgumentsTypes(arguments, ["number"]);
  var newObject = createObject(tulip.Edge, this);
  if (arguments.length == 0) {
    newObject.id = UINT_MAX;
  } else {
    newObject.id = id;
  }
  return newObject;
};
tulip.Edge.prototype.isValid = function() {
  return this.id != UINT_MAX;
};

// ==================================================================================================================

var _getJSONGraph = Module.cwrap('getJSONGraph', 'string', ['number']);
var _loadGraph = Module.cwrap('loadGraph', 'number', ['string', 'number']);
var _saveGraph = Module.cwrap('saveGraph', 'number', ['number', 'string', 'number']);
var _getDefaultAlgorithmParametersJSON = Module.cwrap('getDefaultAlgorithmParametersJSON', 'string', ['string', 'number']);


tulip.loadGraph = function tulip_loadGraph(filename, notifyProgress) {
  checkArgumentsTypes(arguments, ["string", "boolean"]);
  var graphPointer = _loadGraph(filename, notifyProgress);
  if (graphPointer) {
    return tulip.Graph(graphPointer);
  } else {
    return null;
  }
};

tulip.saveGraph = function tulip_saveGraph(graph, filename, notifyProgress) {
  checkArgumentsTypes(arguments, [tulip.Graph, "string"]);
  return _saveGraph(graph.cppPointer, filename, notifyProgress) > 0;
};

tulip.getDefaultAlgorithmParameters = function tulip_getDefaultAlgorithmParameters(algoName, graph) {
  checkArgumentsTypes(arguments, ["string", tulip.Graph], 1);
  var gPointer = 0;
  if (graph) {
    gPointer = graph.cppPointer;
  }
  var params = JSON.parse(_getDefaultAlgorithmParametersJSON(algoName, gPointer));
  for (var property in params) {
    if (params.hasOwnProperty(property)) {
      if (typeOf(params[property]) == "object") {
        if (params[property].type == "property") {
          params[property] = graph.getProperty(params[property].name);
        }
      }
    }
  }
  return params;
};

// ==================================================================================================================

var _algorithmExists = Module.cwrap('algorithmExists', 'number', ['string']);
var _numberOfAlgorithms = Module.cwrap('numberOfAlgorithms', 'number', []);
var _algorithmsNamesLengths = Module.cwrap('algorithmsNamesLengths', 'number', ['number']);
var _algorithmsList = Module.cwrap('algorithmsList', null, ['number']);
var _numberOfPlugins = Module.cwrap('numberOfPlugins', 'number', []);
var _pluginsNamesLengths = Module.cwrap('pluginsNamesLengths', 'number', ['number']);
var _pluginsList = Module.cwrap('pluginsList', null, ['number']);
var _propertyAlgorithmExists = Module.cwrap('propertyAlgorithmExists', 'number', ['string']);
var _booleanAlgorithmExists = Module.cwrap('booleanAlgorithmExists', 'number', ['string']);
var _colorAlgorithmExists = Module.cwrap('colorAlgorithmExists', 'number', ['string']);
var _doubleAlgorithmExists = Module.cwrap('doubleAlgorithmExists', 'number', ['string']);
var _integerAlgorithmExists = Module.cwrap('integerAlgorithmExists', 'number', ['string']);
var _layoutAlgorithmExists = Module.cwrap('layoutAlgorithmExists', 'number', ['string']);
var _stringAlgorithmExists = Module.cwrap('stringAlgorithmExists', 'number', ['string']);
var _sizeAlgorithmExists = Module.cwrap('sizeAlgorithmExists', 'number', ['string']);
var _importPluginExists = Module.cwrap('importPluginExists', 'number', ['string']);

tulip.pluginsList = function() {
  var nbPlugins = _numberOfPlugins();
  var uintArray = _allocArrayInEmHeap(Uint32Array, nbPlugins);
  var nbBytes = _pluginsNamesLengths(uintArray.byteOffset);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  _pluginsList(ucharArray.byteOffset);
  var ret = _bytesTypedArrayToStringArray(ucharArray, uintArray, nbPlugins);
  _freeArrayInEmHeap(ucharArray);
  _freeArrayInEmHeap(uintArray);
  return ret;
};

tulip.algorithmsList = function() {
  var nbAlgorithms = _numberOfAlgorithms();
  var uintArray = _allocArrayInEmHeap(Uint32Array, nbAlgorithms);
  var nbBytes = _pluginsNamesLengths(uintArray.byteOffset);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  _algorithmsList(ucharArray.byteOffset);
  var ret = _bytesTypedArrayToStringArray(ucharArray, uintArray, nbAlgorithms);
  _freeArrayInEmHeap(ucharArray);
  _freeArrayInEmHeap(uintArray);
  return ret;
};

tulip.algorithmExists = function tulip_algoritmExists(algoName) {
  return _algorithmExists(algoName) > 0;
};

tulip.propertyAlgorithmExists = function(algoName) {
  return _propertyAlgorithmExists(algoName) > 0;
};

tulip.booleanAlgorithmExists = function(algoName) {
  return _booleanAlgorithmExists(algoName) > 0;
};

tulip.colorAlgorithmExists = function(algoName) {
  return _colorAlgorithmExists(algoName) > 0;
};

tulip.doubleAlgorithmExists = function(algoName) {
  return _doubleAlgorithmExists(algoName) > 0;
};

tulip.integerAlgorithmExists = function(algoName) {
  return _integerAlgorithmExists(algoName) > 0;
};

tulip.layoutAlgorithmExists = function(algoName) {
  return _layoutAlgorithmExists(algoName) > 0;
};

tulip.stringAlgorithmExists = function(algoName) {
  return _stringAlgorithmExists(algoName) > 0;
};

tulip.sizeAlgorithmExists = function(algoName) {
  return _sizeAlgorithmExists(algoName) > 0;
};

tulip.importPluginExists = function(algoName) {
  return _importPluginExists(algoName) > 0;
};

// ==================================================================================================================

var _Graph_newGraph = Module.cwrap('Graph_newGraph', 'number', []);
var _Graph_delete = Module.cwrap('Graph_delete', null, ['number']);
var _Graph_applyAlgorithm = Module.cwrap('Graph_applyAlgorithm', 'number', ['number', 'string', 'string', 'number']);
var _Graph_clear = Module.cwrap('Graph_clear', null, ['number']);
var _Graph_addSubGraph1 = Module.cwrap('Graph_addSubGraph1', 'number', ['number', 'number', 'string']);
var _Graph_addSubGraph2 = Module.cwrap('Graph_addSubGraph2', 'number', ['number', 'string']);
var _Graph_addCloneSubGraph = Module.cwrap('Graph_addCloneSubGraph', 'number', ['number', 'string', 'number']);
var _Graph_inducedSubGraph = Module.cwrap('Graph_inducedSubGraph', 'number', ['number', 'number', 'number']);
var _Graph_delSubGraph = Module.cwrap('Graph_delSubGraph', null, ['number', 'number']);
var _Graph_delAllSubGraphs = Module.cwrap('Graph_delAllSubGraphs', null, ['number', 'number']);
var _Graph_getSuperGraph = Module.cwrap('Graph_getSuperGraph', 'number', ['number']);
var _Graph_getRoot = Module.cwrap('Graph_getRoot', 'number', ['number']);
var _Graph_getSubGraphs = Module.cwrap('Graph_getSubGraphs', null, ['number', 'number']);
var _Graph_getNthSubGraph = Module.cwrap('Graph_getNthSubGraph', 'number', ['number', 'number']);
var _Graph_numberOfSubGraphs = Module.cwrap('Graph_numberOfSubGraphs', 'number', ['number']);
var _Graph_numberOfDescendantGraphs = Module.cwrap('Graph_numberOfDescendantGraphs', 'number', ['number']);
var _Graph_isSubGraph = Module.cwrap('Graph_isSubGraph', 'number', ['number', 'number']);
var _Graph_isDescendantGraph = Module.cwrap('Graph_isDescendantGraph', 'number', ['number', 'number']);
var _Graph_getSubGraph1 = Module.cwrap('Graph_getSubGraph1', 'number', ['number', 'number']);
var _Graph_getSubGraph2 = Module.cwrap('Graph_getSubGraph2', 'number', ['number', 'string']);
var _Graph_getDescendantGraph1 = Module.cwrap('Graph_getDescendantGraph1', 'number', ['number', 'number']);
var _Graph_getDescendantGraph2 = Module.cwrap('Graph_getDescendantGraph2', 'number', ['number', 'string']);
var _Graph_getDescendantGraphs = Module.cwrap('Graph_getDescendantGraphs', null, ['number', 'number']);

var _Graph_addNode1 = Module.cwrap('Graph_addNode1', 'number', ['number']);
var _Graph_addNode2 = Module.cwrap('Graph_addNode2', null, ['number', 'number']);
var _Graph_addNodes1 = Module.cwrap('Graph_addNodes1', null, ['number', 'number', 'number']);
var _Graph_addNodes2 = Module.cwrap('Graph_addNodes2', null, ['number', 'number', 'number']);
var _Graph_delNode = Module.cwrap('Graph_delNode', null, ['number', 'number', 'number']);
var _Graph_delNodes = Module.cwrap('Graph_delNodes', null, ['number', 'number', 'number', 'number']);

var _Graph_addEdge1 = Module.cwrap('Graph_addEdge1', 'number', ['number', 'number', 'number']);
var _Graph_addEdge2 = Module.cwrap('Graph_addEdge2', null, ['number', 'number']);
var _Graph_addEdges1 = Module.cwrap('Graph_addEdges1', null, ['number', 'number', 'number', 'number']);
var _Graph_addEdges2 = Module.cwrap('Graph_addEdges2', null, ['number', 'number', 'number']);
var _Graph_delEdge = Module.cwrap('Graph_delEdge', null, ['number', 'number', 'number']);
var _Graph_delEdges = Module.cwrap('Graph_delEdges', null, ['number', 'number', 'number', 'number']);
var _Graph_setEdgeOrder = Module.cwrap('Graph_setEdgeOrder', null, ['number', 'number', 'number', 'number']);
var _Graph_swapEdgeOrder = Module.cwrap('Graph_swapEdgeOrder', null, ['number', 'number', 'number', 'number']);
var _Graph_setSource = Module.cwrap('Graph_setSource', null, ['number', 'number', 'number']);
var _Graph_setTarget = Module.cwrap('Graph_setTarget', null, ['number', 'number', 'number']);
var _Graph_setEnds = Module.cwrap('Graph_setEnds', null, ['number', 'number', 'number', 'number']);
var _Graph_reverse = Module.cwrap('Graph_reverse', null, ['number', 'number']);
var _Graph_getSource = Module.cwrap('Graph_getSource', 'number', ['number']);

var _Graph_getNodes = Module.cwrap('Graph_getNodes', null, ['number', 'number']);
var _Graph_getInNodes = Module.cwrap('Graph_getInNodes', null, ['number', 'number', 'number']);
var _Graph_getOutNodes = Module.cwrap('Graph_getOutNodes', null, ['number', 'number', 'number']);
var _Graph_getInOutNodes = Module.cwrap('Graph_getInOutNodes', null, ['number', 'number', 'number']);
var _Graph_bfs = Module.cwrap('Graph_bfs', null, ['number', 'number', 'number']);
var _Graph_dfs = Module.cwrap('Graph_dfs', null, ['number', 'number', 'number']);
var _Graph_getEdges = Module.cwrap('Graph_getEdges', null, ['number', 'number']);
var _Graph_getOutEdges = Module.cwrap('Graph_getOutEdges', null, ['number', 'number', 'number']);
var _Graph_getInOutEdges = Module.cwrap('Graph_getInOutEdges', null, ['number', 'number', 'number']);
var _Graph_getInEdges = Module.cwrap('Graph_getInEdges', null, ['number', 'number', 'number']);

var _Graph_getId = Module.cwrap('Graph_getId', 'number', ['number']);
var _Graph_numberOfNodes = Module.cwrap('Graph_numberOfNodes', 'number', ['number']);
var _Graph_numberOfEdges = Module.cwrap('Graph_numberOfEdges', 'number', ['number']);
var _Graph_deg = Module.cwrap('Graph_deg', 'number', ['number', 'number']);
var _Graph_indeg = Module.cwrap('Graph_indeg', 'number', ['number', 'number']);
var _Graph_outdeg = Module.cwrap('Graph_outdeg', 'number', ['number', 'number']);
var _Graph_source = Module.cwrap('Graph_source', 'number', ['number', 'number']);
var _Graph_target = Module.cwrap('Graph_target', 'number', ['number', 'number']);
var _Graph_ends = Module.cwrap('Graph_ends', null, ['number', 'number', 'number']);
var _Graph_opposite = Module.cwrap('Graph_opposite', 'number', ['number', 'number', 'number']);
var _Graph_isNodeElement = Module.cwrap('Graph_isNodeElement', 'number', ['number', 'number']);
var _Graph_isEdgeElement = Module.cwrap('Graph_isEdgeElement', 'number', ['number', 'number']);
var _Graph_hasEdge = Module.cwrap('Graph_hasEdge', 'number', ['number', 'number', 'number', 'number']);
var _Graph_existEdge = Module.cwrap('Graph_existEdge', 'number', ['number', 'number', 'number', 'number']);
var _Graph_getEdges2 = Module.cwrap('Graph_getEdges2', null, ['number', 'number', 'number', 'number', 'number']);

var _Graph_getName = Module.cwrap('Graph_getName', 'string', ['number']);
var _Graph_setName = Module.cwrap('Graph_setName', null, ['number', 'string']);

var _Graph_numberOfProperties = Module.cwrap('Graph_numberOfProperties', 'number', ['number']);
var _Graph_propertiesNamesLengths = Module.cwrap('Graph_propertiesNamesLengths', 'number', ['number', 'number']);
var _Graph_getProperties = Module.cwrap('Graph_getProperties', null, ['number', 'number']);

var _Graph_getProperty = Module.cwrap('Graph_getProperty', 'number', ['number', 'string']);
var _Graph_getBooleanProperty = Module.cwrap('Graph_getBooleanProperty', 'number', ['number', 'string']);
var _Graph_getColorProperty = Module.cwrap('Graph_getColorProperty', 'number', ['number', 'string']);
var _Graph_getDoubleProperty = Module.cwrap('Graph_getDoubleProperty', 'number', ['number', 'string']);
var _Graph_getIntegerProperty = Module.cwrap('Graph_getIntegerProperty', 'number', ['number', 'string']);
var _Graph_getLayoutProperty = Module.cwrap('Graph_getLayoutProperty', 'number', ['number', 'string']);
var _Graph_getSizeProperty = Module.cwrap('Graph_getSizeProperty', 'number', ['number', 'string']);
var _Graph_getStringProperty = Module.cwrap('Graph_getStringProperty', 'number', ['number', 'string']);
var _Graph_getBooleanVectorProperty = Module.cwrap('Graph_getBooleanVectorProperty', 'number', ['number', 'string']);
var _Graph_getColorVectorProperty = Module.cwrap('Graph_getColorVectorProperty', 'number', ['number', 'string']);
var _Graph_getDoubleVectorProperty = Module.cwrap('Graph_getDoubleVectorProperty', 'number', ['number', 'string']);
var _Graph_getIntegerVectorProperty = Module.cwrap('Graph_getIntegerVectorProperty', 'number', ['number', 'string']);
var _Graph_getCoordVectorProperty = Module.cwrap('Graph_getCoordVectorProperty', 'number', ['number', 'string']);
var _Graph_getSizeVectorProperty = Module.cwrap('Graph_getSizeVectorProperty', 'number', ['number', 'string']);
var _Graph_getStringVectorProperty = Module.cwrap('Graph_getStringVectorProperty', 'number', ['number', 'string']);
var _Graph_delLocalProperty = Module.cwrap('Graph_delLocalProperty', null, ['number', 'string']);
var _Graph_applyPropertyAlgorithm = Module.cwrap('Graph_applyPropertyAlgorithm', 'number', ['number', 'string', 'number', 'string', 'number']);
var _Graph_push = Module.cwrap('Graph_push', null, ['number']);
var _Graph_pop = Module.cwrap('Graph_pop', null, ['number']);
var _Graph_setEventsActivated = Module.cwrap('Graph_setEventsActivated', null, ['number', 'number']);
var _Graph_getNodesPropertiesValuesJSON = Module.cwrap('Graph_getNodesPropertiesValuesJSON', 'string', ['number']);
var _Graph_getEdgesPropertiesValuesJSON = Module.cwrap('Graph_getEdgesPropertiesValuesJSON', 'string', ['number']);
var _Graph_getAttributesJSON = Module.cwrap('Graph_getAttributesJSON', 'string', ['number']);


/**
* This is the description for the tulip.Graph class.
*
* @class Graph
* @constructor
*/
tulip.Graph = function tulip_Graph(cppPointer) {
  var newObject = createObject(tulip.Graph, this);
  if (arguments.length == 0) {
    tulip.CppObjectWrapper.call(newObject, _Graph_newGraph(), "tlp::Graph");
  } else {
    tulip.CppObjectWrapper.call(newObject, cppPointer, "tlp::Graph");
  }
  if (!workerMode) {
    _graphIdToWrapper[newObject.getCppPointer()] = newObject;
  }
  return newObject;
};
tulip.Graph.inheritsFrom(tulip.CppObjectWrapper);
tulip.Graph.prototype.destroy = function() {
  _Graph_delete(this.cppPointer);
  this.cppPointer = 0;
};
tulip.Graph.prototype.applyAlgorithm = function tulip_Graph_prototype_applyAlgorithm(algorithmName, algoParameters, notifyProgress) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", "object", "boolean"], 1);
  if (!tulip.algorithmExists(algorithmName)) {
    console.log("Error : no Tulip algorithm named '" + algorithmName + "'");
    return false;
  }
  if (algoParameters == undefined) algoParameters = {};
  if (notifyProgress == undefined) notifyProgress = false;
  return _Graph_applyAlgorithm(this.cppPointer, algorithmName, JSON.stringify(algoParameters), notifyProgress) > 0;
};
tulip.Graph.prototype.clear = function tulip_Graph_prototype_clear() {
  checkWrappedCppPointer(this.cppPointer);
  _Graph_clear(this.cppPointer);
};
tulip.Graph.prototype.addSubGraph = function tulip_Graph_prototype_addSubGraph() {
  checkWrappedCppPointer(this.cppPointer);
  // addSubGraph(selection, name)
  if (arguments.length == 2) {
    checkArgumentsTypes(arguments, [tulip.BooleanProperty, "string"]);
    return tulip.Graph(_Graph_addSubGraph1(this.cppPointer, arguments[0].cppPointer, arguments[1]));
    // addSubGraph(name)
  } else if (arguments.length == 1) {
    checkArgumentsTypes(arguments, ["string"]);
    return tulip.Graph(_Graph_addSubGraph2(this.cppPointer, arguments[0]));
  } else if (arguments.length == 0) {
    return tulip.Graph(_Graph_addSubGraph2(this.cppPointer, ""));
  } else {
    return null;
  }
};
tulip.Graph.prototype.addCloneSubGraph = function tulip_Graph_prototype_addCloneSubGraph(name, addSibling) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", "boolean"]);
  return tulip.Graph(_Graph_addCloneSubGraph(this.cppPointer, name, addSibling));
};
tulip.Graph.prototype.inducedSubGraph = function tulip_Graph_prototype_inducedSubGraph(nodes, parentSubGraph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array", tulip.Graph]);
  checkArrayOfType(nodes, tulip.Node, 0);
  var nodesIdsArray = _allocArrayInEmHeap(Uint32Array, nodes.length+1);
  for (var i = 0 ; i < nodes.length ; ++i) {
    nodesIdsArray[i] = nodes[i].id;
  }
  nodesIdsArray[nodes.length] = UINT_MAX;
  var sg = tulip.Graph(_Graph_inducedSubGraph(this.cppPointer, nodesIdsArray.byteOffset, parentSubGraph ? parentSubGraph.cppPointer : 0));
  _freeArrayInEmHeap(nodesIdsArray);
  return sg;
};
tulip.Graph.prototype.delSubGraph = function tulip_Graph_prototype_delSubGraph(sg) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Graph]);
  _Graph_delSubGraph(this.cppPointer, sg.cppPointer);
};
tulip.Graph.prototype.delAllSubGraphs = function tulip_Graph_prototype_delAllSubGraphs(sg) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Graph]);
  _Graph_delAllSubGraphs(this.cppPointer, sg.cppPointer);
};
tulip.Graph.prototype.getSuperGraph = function tulip_Graph_prototype_getSuperGraph() {
  checkWrappedCppPointer(this.cppPointer);
  return tulip.Graph(_Graph_getSuperGraph(this.cppPointer));
};
tulip.Graph.prototype.getRoot = function tulip_Graph_prototype_getRoot() {
  checkWrappedCppPointer(this.cppPointer);
  return tulip.Graph(_Graph_getRoot(this.cppPointer));
};
tulip.Graph.prototype.getSubGraphs = function tulip_Graph_prototype_getSubGraphs() {
  checkWrappedCppPointer(this.cppPointer);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.numberOfSubGraphs(), function(byteOffset){_Graph_getSubGraphs(graphObject.cppPointer, byteOffset)}, tulip.Graph);
};
tulip.Graph.prototype.getNthSubGraph = function tulip_Graph_prototype_getNthSubGraph(n) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["number"]);
  return tulip.Graph(_Graph_getNthSubGraph(this.cppPointer, n));
};
tulip.Graph.prototype.numberOfSubGraphs = function tulip_Graph_prototype_numberOfSubGraphs() {
  checkWrappedCppPointer(this.cppPointer);
  return _Graph_numberOfSubGraphs(this.cppPointer);
};
tulip.Graph.prototype.numberOfDescendantGraphs = function tulip_Graph_prototype_numberOfDescendantGraphs() {
  checkWrappedCppPointer(this.cppPointer);
  return _Graph_numberOfDescendantGraphs(this.cppPointer);
};
tulip.Graph.prototype.isSubGraph = function tulip_Graph_prototype_isSubGraph(subGraph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Graph], 1);
  return _Graph_isSubGraph(this.cppPointer, subGraph.cppPointer);
};
tulip.Graph.prototype.isDescendantGraph = function tulip_Graph_prototype_isDescendantGraph(subGraph) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Graph], 1);
  return _Graph_isDescendantGraph(this.cppPointer, subGraph.cppPointer);
};
tulip.Graph.prototype.getSubGraph = function tulip_Graph_prototype_getSubGraph() {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [["number", "string"]], 1);
  if (typeOf(arguments[0]) == "string") {
    return tulip.Graph(_Graph_getSubGraph2(this.cppPointer, arguments[0]));
  } else {
    return tulip.Graph(_Graph_getSubGraph1(this.cppPointer, arguments[0]));
  }
};
tulip.Graph.prototype.getDescendantGraph = function tulip_Graph_prototype_getDescendantGraph() {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [["number", "string"]], 1);
  if (typeOf(arguments[0]) == "string") {
    return tulip.Graph(_Graph_getDescendantGraph2(this.cppPointer, arguments[0]));
  } else {
    return tulip.Graph(_Graph_getDescendantGraph1(this.cppPointer, arguments[0]));
  }
};
tulip.Graph.prototype.getDescendantGraphs = function tulip_Graph_prototype_getDescendantGraphs() {
  checkWrappedCppPointer(this.cppPointer);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.numberOfDescendantGraphs(), function(byteOffset){_Graph_getDescendantGraphs(graphObject.cppPointer, byteOffset)}, tulip.Graph);
};
tulip.Graph.prototype.addNode = function tulip_Graph_prototype_addNode() {
  checkWrappedCppPointer(this.cppPointer);
  if (arguments.length == 0) {
    return tulip.Node(_Graph_addNode1(this.cppPointer));
  } else {
    checkArgumentsTypes(arguments, [tulip.Node], 1);
    _Graph_addNode2(this.cppPointer, arguments[0].id);
  }
};
tulip.Graph.prototype.addNodes = function tulip_Graph_prototype_addNodes() {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [['number', 'array']], 1);
  // addNodes(nbNodes) -> array of tulip.Node
  if (typeOf(arguments[0]) == 'number') {
    var nbNodes = arguments[0];
    var graphObject = this;
    return getArrayOfTulipType(nbNodes, function(byteOffset){_Graph_addNodes1(graphObject.cppPointer, nbNodes, byteOffset)}, tulip.Node);

    // addNodes(array of tulip.Node)
  } else {
    var nodes = arguments[0];
    checkArrayOfType(nodes, tulip.Node, 0);
    var nodesIdsArray = _allocArrayInEmHeap(Uint32Array, nodes.length);
    for (var i = 0 ; i < nodes.length ; ++i) {
      nodesIdsArray[i] = nodes[i].id;
    }
    _Graph_addNodes2(this.cppPointer, nodes.length, nodesIdsArray.byteOffset);
    _freeArrayInEmHeap(nodesIdsArray);
  }
};
tulip.Graph.prototype.delNode = function tulip_Graph_prototype_delNode(node, deleteInAllGraphs) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "boolean"], 1);
  if (arguments.length == 1) {
    deleteInAllGraphs = false;
  }
  _Graph_delNode(this.cppPointer, node.id, deleteInAllGraphs);
};
tulip.Graph.prototype.delNodes = function tulip_Graph_prototype_delNodes(nodes, deleteInAllGraphs) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array", "boolean"], 1);
  checkArrayOfType(nodes, tulip.Node, 0);
  if (arguments.length == 1) {
    deleteInAllGraphs = false;
  }
  var nodesIdsArray = _allocArrayInEmHeap(Uint32Array, nodes.length);
  for (var i = 0 ; i < nodes.length ; ++i) {
    nodesIdsArray[i] = nodes[i].id;
  }
  _Graph_delNodes(this.cppPointer, nodes.length, nodesIdsArray.byteOffset, deleteInAllGraphs);
  _freeArrayInEmHeap(nodesIdsArray);
};
/**
* Adds a new edge in the graph
*
* @method addEdge
* @param {Node} src the source node
* @param {Node} tgt the target node
* @return {Edge} Returns a new graph edge
*/
tulip.Graph.prototype.addEdge = function tulip_Graph_prototype_addEdge() {
  checkWrappedCppPointer(this.cppPointer);
  // addEdge(tulip.Edge)
  if (arguments.length <= 1) {
    checkArgumentsTypes(arguments, [tulip.Edge], 1);
    var edge = arguments[0];
    _Graph_addEdge2(this.cppPointer, edge.id);
    // addEdge(tulip.Node, tulip.Node) -> tulip.Edge
  } else {
    checkArgumentsTypes(arguments, [tulip.Node, tulip.Node], 2);
    var src = arguments[0];
    var tgt = arguments[1];
    return tulip.Edge(_Graph_addEdge1(this.cppPointer, src.id, tgt.id));
  }
};
tulip.Graph.prototype.addEdges = function tulip_Graph_prototype_addEdges() {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"], 1);
  try {
    // addEdge(array of [tulip.Node, tulip.Node])
    checkArrayOfType(arguments[0], "array");
    try {
      var nodes = arguments[0];
      for (var i = 0 ; i < nodes.length ; ++i) {
        if (nodes[i].length != 2) {
          throw "error";
        } else {
          checkArrayOfType(nodes[i], tulip.Node);
        }
      }
      var nodesIdsArray = _allocArrayInEmHeap(Uint32Array, nodes.length*2);
      for (var i = 0 ; i < nodes.length ; ++i) {
        nodesIdsArray[2*i] = nodes[i][0].id;
        nodesIdsArray[2*i+1] = nodes[i][1].id;
      }
      var graphObject = this;
      var edges = getArrayOfTulipType(nodes.length, function(byteOffset){_Graph_addEdges1(graphObject.cppPointer, nodes.length, nodesIdsArray.byteOffset, byteOffset)}, tulip.Edge);
      _freeArrayInEmHeap(nodesIdsArray);
      return edges;
    } catch (err) {
      throw new TypeError("Error when calling function 'tulip.Graph.prototype.addEdges', parameter 0 must be an array of arrays containing two instances of tulip.Node objects");
    }
  } catch (err) {
    // addEdge(array of tulip.Edge)
    checkArrayOfType(arguments[0], tulip.Edge);
    var edges = arguments[0];
    var edgesIdsArray = _allocArrayInEmHeap(Uint32Array, edges.length);
    for (var i = 0 ; i < edges.length ; ++i) {
      edgesIdsArray[i] = edges[i].id;
    }
    _Graph_addEdges2(this.cppPointer, edges.length, edgesIdsArray.byteOffset);
    _freeArrayInEmHeap(edgesIdsArray);
  }
};
tulip.Graph.prototype.delEdge = function tulip_Graph_prototype_delEdge(edge, deleteInAllGraphs) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, "boolean"], 1);
  if (arguments.length == 1) {
    deleteInAllGraphs = false;
  }
  _Graph_delEdge(this.cppPointer, edge.id, deleteInAllGraphs);
};
tulip.Graph.prototype.delEdges = function tulip_Graph_prototype_delEdges(edges, deleteInAllGraphs) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array", "boolean"], 1);
  checkArrayOfType(edges, tulip.Edge, 0);
  if (arguments.length == 1) {
    deleteInAllGraphs = false;
  }
  var edgesIdsArray = _allocArrayInEmHeap(Uint32Array, edges.length);
  for (var i = 0 ; i < edges.length ; ++i) {
    edgesIdsArray[i] = edges[i].id;
  }
  _Graph_delEdges(this.cppPointer, edges.length, edgesIdsArray.byteOffset, deleteInAllGraphs);
  _freeArrayInEmHeap(edgesIdsArray);
};
tulip.Graph.prototype.setEdgeOrder = function tulip_Graph_prototype_setEdgeOrder(node, edges) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, "array"], 2);
  checkArrayOfType(edges, tulip.Edge);
  var edgesIdsArray = _allocArrayInEmHeap(Uint32Array, edges.length);
  for (var i = 0 ; i < edges.length ; ++i) {
    edgesIdsArray[i] = edges[i].id;
  }
  _Graph_setEdgeOrder(this.cppPointer, node.id, edges.length, edgesIdsArray.byteOffset);
  _freeArrayInEmHeap(edgesIdsArray);
};
tulip.Graph.prototype.swapEdgeOrder = function tulip_Graph_prototype_swapEdgeOrder(node, edge1, edge2) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, tulip.Edge, tulip.Edge], 3);
  _Graph_swapEdgeOrder(this.cppPointer, node.id, edge1.id, edge2.id);
};
tulip.Graph.prototype.setSource = function tulip_Graph_prototype_setSource(edge, node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, tulip.Node], 2);
  _Graph_setSource(this.cppPointer, edge.id, node.id);
};
tulip.Graph.prototype.setTarget = function tulip_Graph_prototype_setTarget(edge, node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, tulip.Node], 2);
  _Graph_setTarget(this.cppPointer, edge.id, node.id);
};
tulip.Graph.prototype.setEnds = function tulip_Graph_prototype_setEnds(edge, source, target) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, tulip.Node, tulip.Node], 3);
  _Graph_setEnds(this.cppPointer, edge.id, source.id, target.id);
};
tulip.Graph.prototype.reverse = function tulip_Graph_prototype_reverse(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge], 1);
  _Graph_reverse(this.cppPointer, edge.id);
};
tulip.Graph.prototype.getSource = function tulip_Graph_prototype_getSource() {
  checkWrappedCppPointer(this.cppPointer);
  return tulip.Node(_Graph_getSource(this.cppPointer));
};
tulip.Graph.prototype.getNodes = function tulip_Graph_prototype_getNodes() {
  checkWrappedCppPointer(this.cppPointer);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.numberOfNodes(), function(byteOffset){_Graph_getNodes(graphObject.cppPointer, byteOffset)}, tulip.Node);
};
tulip.Graph.prototype.getRandomNode = function tulip_Graph_prototype_getRandomNode() {
  checkWrappedCppPointer(this.cppPointer);
  var idx = (Math.random() * this.numberOfNodes()) | 0;
  var nodes = this.getNodes();
  return nodes[idx];
};
tulip.Graph.prototype.getInNodes = function tulip_Graph_prototype_getInNodes(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.indeg(node), function(byteOffset){_Graph_getInNodes(graphObject.cppPointer, node.id, byteOffset)}, tulip.Node);
};
tulip.Graph.prototype.getOutNodes = function tulip_Graph_prototype_getOutNodes(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.outdeg(node), function(byteOffset){_Graph_getOutNodes(graphObject.cppPointer, node.id, byteOffset)}, tulip.Node);
};
tulip.Graph.prototype.getInOutNodes = function tulip_Graph_prototype_getInOutNodes(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.deg(node), function(byteOffset){_Graph_getInOutNodes(graphObject.cppPointer, node.id, byteOffset)}, tulip.Node);
};
tulip.Graph.prototype.bfs = function tulip_Graph_prototype_bfs(root) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  if (arguments.length == 0) {
    return getArrayOfTulipType(graphObject.numberOfNodes(), function(byteOffset){_Graph_bfs(graphObject.cppPointer, UINT_MAX, byteOffset)}, tulip.Node);
  } else {
    return getArrayOfTulipType(graphObject.numberOfNodes(), function(byteOffset){_Graph_bfs(graphObject.cppPointer, root.id, byteOffset)}, tulip.Node);
  }
};
tulip.Graph.prototype.dfs = function tulip_Graph_prototype_dfs(root) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  if (arguments.length == 0) {
    return getArrayOfTulipType(graphObject.numberOfNodes(), function(byteOffset){_Graph_dfs(graphObject.cppPointer, UINT_MAX, byteOffset)}, tulip.Node);
  } else {
    return getArrayOfTulipType(graphObject.numberOfNodes(), function(byteOffset){_Graph_dfs(graphObject.cppPointer, root.id, byteOffset)}, tulip.Node);
  }
};
tulip.Graph.prototype.getEdges = function tulip_Graph_prototype_getEdges() {
  checkWrappedCppPointer(this.cppPointer);
  // getEdges()
  if (arguments.length == 0) {
    var graphObject = this;
    return getArrayOfTulipType(graphObject.numberOfEdges(), function(byteOffset){_Graph_getEdges(graphObject.cppPointer, byteOffset)}, tulip.Edge);
    // getEdges(tulip.Node, tulip.Node, boolean)
  } else {
    checkArgumentsTypes(arguments, [tulip.Node, tulip.Node, "boolean"], 2);
    var directed = true;
    if (arguments.length > 2) directed = arguments[2];
    return getArrayOfTulipType(graphObject.deg(arguments[0]), function(byteOffset){_Graph_getEdges2(this.cppPointer, arguments[0].id, arguments[1].id, directed, byteOffset)}, tulip.Edge);
  }
};
tulip.Graph.prototype.getRandomEdge = function tulip_Graph_prototype_getRandomEdge() {
  checkWrappedCppPointer(this.cppPointer);
  var idx = (Math.random() * this.numberOfEdges()) | 0;
  var edges = this.getEdges();
  return edges[idx];
};
tulip.Graph.prototype.getOutEdges = function tulip_Graph_prototype_getOutEdges(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.outdeg(node), function(byteOffset){_Graph_getOutEdges(graphObject.cppPointer, node.id, byteOffset)}, tulip.Edge);
};
tulip.Graph.prototype.getInOutEdges = function tulip_Graph_prototype_getInOutEdges(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.deg(node), function(byteOffset){_Graph_getInOutEdges(graphObject.cppPointer, node.id, byteOffset)}, tulip.Edge);
};
tulip.Graph.prototype.getInEdges = function tulip_Graph_prototype_getInEdges(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  var graphObject = this;
  return getArrayOfTulipType(graphObject.indeg(node), function(byteOffset){_Graph_getInEdges(graphObject.cppPointer, node.id, byteOffset)}, tulip.Edge);
};
tulip.Graph.prototype.getId = function tulip_Graph_prototype_getId() {
  checkWrappedCppPointer(this.cppPointer);
  return _Graph_getId(this.cppPointer);
};
tulip.Graph.prototype.numberOfNodes = function tulip_Graph_prototype_numberOfNodes() {
  checkWrappedCppPointer(this.cppPointer);
  return _Graph_numberOfNodes(this.cppPointer);
};
tulip.Graph.prototype.numberOfEdges = function tulip_Graph_prototype_numberOfEdges() {
  checkWrappedCppPointer(this.cppPointer);
  return _Graph_numberOfEdges(this.cppPointer);
};
tulip.Graph.prototype.deg = function tulip_Graph_prototype_deg(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  return _Graph_deg(this.cppPointer, node.id);
};
tulip.Graph.prototype.indeg = function tulip_Graph_prototype_indeg(node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node]);
  return _Graph_indeg(this.cppPointer, node.id);
};
tulip.Graph.prototype.outdeg = function tulip_Graph_prototype_outdeg(node) {
  checkArgumentsTypes(arguments, [tulip.Node]);
  return _Graph_outdeg(this.cppPointer, node.id);
};
tulip.Graph.prototype.source = function tulip_Graph_prototype_source(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  return tulip.Node(_Graph_source(this.cppPointer, edge.id));
};
tulip.Graph.prototype.target = function tulip_Graph_prototype_target(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  return tulip.Node(_Graph_target(this.cppPointer, edge.id));
};
tulip.Graph.prototype.ends = function tulip_Graph_prototype_ends(edge) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge]);
  return getArrayOfTulipType(2, function(byteOffset){_Graph_ends(graphObject.cppPointer, edge.id, byteOffset)}, tulip.Node);
};
tulip.Graph.prototype.opposite = function tulip_Graph_prototype_opposite(edge, node) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Edge, tulip.Node]);
  return tulip.Node(_Graph_opposite(this.cppPointer, edge.id, node.id));
};
tulip.Graph.prototype.isElement = function tulip_Graph_prototype_isElement(elem) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [[tulip.Edge, tulip.Node]]);
  if (elem instanceof tulip.Node) {
    return _Graph_isNodeElement(this.cppPointer, elem.id) > 0;
  } else {
    return _Graph_isEdgeElement(this.cppPointer, elem.id) > 0;
  }
};
tulip.Graph.prototype.hasEdge = function tulip_Graph_prototype_hasEdge(sourceNode, targetNode, directed) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, tulip.Node, "boolean"], 2);
  if (directed == undefined) directed = true;
  return _Graph_hasEdge(this.cppPointer, sourceNode.id, targetNode.id, directed) > 0;
};
tulip.Graph.prototype.existEdge = function tulip_Graph_prototype_existEdge(sourceNode, targetNode, directed) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, [tulip.Node, tulip.Node, "boolean"], 2);
  if (directed == undefined) directed = true;
  return tulip.Edge(_Graph_existEdge(this.cppPointer, sourceNode.id, targetNode.id, directed));
};
tulip.Graph.prototype.getName = function tulip_Graph_prototype_getName() {
  checkWrappedCppPointer(this.cppPointer);
  return _Graph_getName(this.cppPointer);
};
tulip.Graph.prototype.setName = function tulip_Graph_prototype_setName(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  _Graph_setName(this.cppPointer, name);
};
tulip.Graph.prototype.getProperties = function tulip_Graph_prototype_getProperties() {
  checkWrappedCppPointer(this.cppPointer);
  var nbProperties = _Graph_numberOfProperties(this.cppPointer);
  var uintArray = _allocArrayInEmHeap(Uint32Array, nbProperties);
  var nbBytes = _Graph_propertiesNamesLengths(this.cppPointer, uintArray.byteOffset);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbBytes);
  _Graph_getProperties(this.cppPointer, ucharArray.byteOffset);
  var ret = _bytesTypedArrayToStringArray(ucharArray, uintArray, nbProperties);
  _freeArrayInEmHeap(uintArray);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};
tulip.Graph.prototype.getProperty = function tulip_Graph_prototype_getPropery(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  var typedProperty = null;
  var propertyPointer = _Graph_getProperty(this.cppPointer, name);
  if (propertyPointer) {
    var prop = tulip.PropertyInterface(propertyPointer, true);
    var propertyType = prop.getTypename();
    switch (propertyType) {
    case 'bool':
      typedProperty = this.getBooleanProperty(name);
      break;
    case 'color':
      typedProperty = this.getColorProperty(name);
      break;
    case 'double':
      typedProperty = this.getDoubleProperty(name);
      break;
    case 'int' :
      typedProperty = this.getIntegerProperty(name);
      break;
    case 'layout':
      typedProperty = this.getLayoutProperty(name);
      break;
    case 'size':
      typedProperty = this.getSizeProperty(name);
      break;
    case 'string':
      typedProperty = this.getStringProperty(name);
      break;
    case 'vector<bool>':
      typedProperty = this.getBooleanVectorProperty(name);
      break;
    case 'vector<color>':
      typedProperty = this.getColorVectorProperty(name);
      break;
    case 'vector<double>':
      typedProperty = this.getDoubleVectorProperty(name);
      break;
    case 'vector<int>' :
      typedProperty = this.getIntegerVectorProperty(name);
      break;
    case 'vector<coord>':
      typedProperty = this.getCoordVectorProperty(name);
      break;
    case 'vector<size>':
      typedProperty = this.getSizeVectorProperty(name);
      break;
    case 'vector<string>':
      typedProperty = this.getStringVectorProperty(name);
      break;
    default:
      typedProperty = prop;
      break;
    }
  }
  return typedProperty;
};
tulip.Graph.prototype.getBooleanProperty = function tulip_Graph_prototype_getBooleanProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.BooleanProperty(_Graph_getBooleanProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getBooleanVectorProperty = function tulip_Graph_prototype_getBooleanVectorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.BooleanVectorProperty(_Graph_getBooleanVectorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getColorProperty = function tulip_Graph_prototype_getColorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.ColorProperty(_Graph_getColorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getColorVectorProperty = function tulip_Graph_prototype_getColorVectorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.ColorVectorProperty(_Graph_getColorVectorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getDoubleProperty = function tulip_Graph_prototype_getDoubleProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.DoubleProperty(_Graph_getDoubleProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getDoubleVectorProperty = function tulip_Graph_prototype_getDoubleVectorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.DoubleVectorProperty(_Graph_getDoubleVectorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getIntegerProperty = function tulip_Graph_prototype_getIntegerProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.IntegerProperty(_Graph_getIntegerProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getIntegerVectorProperty = function tulip_Graph_prototype_getIntegerVectorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.IntegerVectorProperty(_Graph_getIntegerVectorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getLayoutProperty = function tulip_Graph_prototype_getLayoutProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.LayoutProperty(_Graph_getLayoutProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getCoordVectorProperty = function tulip_Graph_prototype_getCoordVectorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.CoordVectorProperty(_Graph_getCoordVectorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getSizeProperty = function tulip_Graph_prototype_getSizeProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.SizeProperty(_Graph_getSizeProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getSizeVectorProperty = function tulip_Graph_prototype_getSizeVectorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.SizeVectorProperty(_Graph_getSizeVectorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getStringProperty = function tulip_Graph_prototype_getStringProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.StringProperty(_Graph_getStringProperty(this.cppPointer, name));
};
tulip.Graph.prototype.getStringVectorProperty = function tulip_Graph_prototype_getStringVectorProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"]);
  return tulip.StringVectorProperty(_Graph_getStringVectorProperty(this.cppPointer, name));
};
tulip.Graph.prototype.delLocalProperty = function tulip_Graph_prototype_delLocalProperty(name) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string"], 1);
  _Graph_delLocalProperty(this.cppPointer, name);
};
tulip.Graph.prototype.toJSON = function tulip_Graph_prototype_toJSON() {
  checkWrappedCppPointer(this.cppPointer);
  return _getJSONGraph(this.cppPointer);
};
tulip.Graph.prototype.getNodesPropertiesValues = function tulip_Graph_prototype_getNodesPropertiesValues() {
  return JSON.parse(_Graph_getNodesPropertiesValuesJSON(this.cppPointer));
};
tulip.Graph.prototype.getEdgesPropertiesValues = function tulip_Graph_prototype_getEdgesPropertiesValues() {
  return JSON.parse(_Graph_getEdgesPropertiesValuesJSON(this.cppPointer));
};
tulip.Graph.prototype.applyPropertyAlgorithm = function tulip_Graph_prototype_applyPropertyAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", tulip.PropertyInterface, "object", "boolean"], 2);
  if (!tulip.propertyAlgorithmExists(algorithmName)) {
    console.log("Error : no Tulip property algorithm named '" + algorithmName + "'");
    return false;
  }
  if (algoParameters === undefined) algoParameters = {};
  if (notifyProgress == undefined) notifyProgress = false;
  return _Graph_applyPropertyAlgorithm(this.cppPointer, algorithmName, resultProperty.cppPointer, JSON.stringify(algoParameters), notifyProgress) > 0;
};

tulip.Graph.prototype.applyBooleanAlgorithm = function tulip_Graph_prototype_applyBooleanAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", tulip.BooleanProperty, "object", "boolean"], 2);
  if (!tulip.booleanAlgorithmExists(algorithmName)) {
    console.log("Error : no Tulip boolean algorithm named '" + algorithmName + "'");
    return false;
  }
  if (algoParameters === undefined) algoParameters = {};
  if (notifyProgress == undefined) notifyProgress = false;
  return this.applyPropertyAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress);
};

tulip.Graph.prototype.applyColorAlgorithm = function tulip_Graph_prototype_applyColorAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", tulip.ColorProperty, "object", "boolean"], 2);
  if (!tulip.colorAlgorithmExists(algorithmName)) {
    console.log("Error : no Tulip color algorithm named '" + algorithmName + "'");
    return false;
  }
  if (algoParameters === undefined) algoParameters = {};
  if (notifyProgress == undefined) notifyProgress = false;
  return this.applyPropertyAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress);
};

tulip.Graph.prototype.applyDoubleAlgorithm = function tulip_Graph_prototype_applyDoubleAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", tulip.DoubleProperty, "object", "boolean"], 2);
  if (!tulip.doubleAlgorithmExists(algorithmName)) {
    console.log("Error : no Tulip double algorithm named '" + algorithmName + "'");
    return false;
  }
  if (algoParameters === undefined) algoParameters = {};
  if (notifyProgress == undefined) notifyProgress = false;
  return this.applyPropertyAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress);
};

tulip.Graph.prototype.applyLayoutAlgorithm = function tulip_Graph_prototype_applyLayoutAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", tulip.LayoutProperty, "object", "boolean"], 2);
  if (!tulip.layoutAlgorithmExists(algorithmName)) {
    console.log("Error : no Tulip layout algorithm named '" + algorithmName + "'");
    return false;
  }
  if (algoParameters === undefined) algoParameters = {};
  if (notifyProgress == undefined) notifyProgress = false;
  return this.applyPropertyAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress);
};

tulip.Graph.prototype.applySizeAlgorithm = function tulip_Graph_prototype_applySizeAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["string", tulip.SizeProperty, "object", "boolean"], 2);
  if (!tulip.sizeAlgorithmExists(algorithmName)) {
    console.log("Error : no Tulip size algorithm named '" + algorithmName + "'");
    return false;
  }
  if (algoParameters === undefined) algoParameters = {};
  if (notifyProgress == undefined) notifyProgress = false;
  return this.applyPropertyAlgorithm(algorithmName, resultProperty, algoParameters, notifyProgress);
};

tulip.Graph.prototype.push = function tulip_Graph_prototype_push() {
  checkWrappedCppPointer(this.cppPointer);
  _Graph_push(this.cppPointer);
};

tulip.Graph.prototype.pop = function tulip_Graph_prototype_pop() {
  checkWrappedCppPointer(this.cppPointer);
  _Graph_pop(this.cppPointer);
};

tulip.Graph.prototype.setEventsActivated = function tulip_Graph_prototype_setEventsActivated(eventsActivated) {
  checkWrappedCppPointer(this.cppPointer);
  _Graph_setEventsActivated(this.cppPointer, eventsActivated);
};

tulip.Graph.prototype.getAttributes = function tulip_Graph_prototype_getAttributes() {
  checkWrappedCppPointer(this.cppPointer);
  return JSON.parse(_Graph_getAttributesJSON(this.cppPointer));
}

tulip.Graph.prototype.getAttribute = function tulip_Graph_prototype_getAttribute(name) {
  checkWrappedCppPointer(this.cppPointer);
  var attributes = this.getAttributes();
  if (name in attributes) {
    return attributes[name];
  } else {
    return undefined;
  }
}

// ==================================================================================================================

var _ColorScale_newColorScale = Module.cwrap('ColorScale_newColorScale', 'number', []);
var _ColorScale_setColorScale = Module.cwrap('ColorScale_setColorScale', null, ['number', 'number', 'number']);
var _ColorScale_setColorAtPos = Module.cwrap('ColorScale_setColorAtPos', null, ['number', 'number', 'number', 'number', 'number', 'number']);
var _ColorScale_getColorAtPos = Module.cwrap('ColorScale_getColorAtPos', null, ['number', 'number', 'number']);
var _ColorScale_numberOfColors = Module.cwrap('ColorScale_numberOfColors', 'number', ['number']);
var _ColorScale_getOffsets = Module.cwrap('ColorScale_getOffsets', null, ['number', 'number']);
var _ColorScale_getColors = Module.cwrap('ColorScale_getColors', null, ['number', 'number']);

tulip.ColorScale = function tulip_ColorScale() {
  var newObject = createObject(tulip.ColorScale, this);
  tulip.CppObjectWrapper.call(newObject, _ColorScale_newColorScale(), "tlp::ColorScale");
  if (arguments.length == 1) {
    checkArgumentsTypes(arguments, ["array"]);
    checkArrayOfType(arguments[0], tulip.Color);
    newObject.setColorScale(arguments[0]);
  }

  return newObject;
};

tulip.ColorScale.prototype.setColorScale = function tulip_ColorScale_prototype_setColorScale(colors) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["array"], 1);
  checkArrayOfType(colors, tulip.Color, 0);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, colors.length * 4);
  for (var i = 0 ; i < colors.length ; ++i) {
    ucharArray[4*i] = colors[i].r;
    ucharArray[4*i+1] = colors[i].g;
    ucharArray[4*i+2] = colors[i].b;
    ucharArray[4*i+3] = colors[i].a;
  }
  _ColorScale_setColorScale(this.cppPointer, ucharArray.byteOffset, colors.length);
  _freeArrayInEmHeap(ucharArray);
};

tulip.ColorScale.prototype.setColorAtPos = function tulip_ColorScale_prototype_setColorAtPos(pos, color) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["number", tulip.Color], 2);
  _ColorScale_setColorAtPos(this.cppPointer, pos, color.r, color.g, color.b, color.a);
};

tulip.ColorScale.prototype.getColorAtPos = function tulip_ColorScale_prototype_setColorAtPos(pos) {
  checkWrappedCppPointer(this.cppPointer);
  checkArgumentsTypes(arguments, ["number"], 1);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, 4);
  _ColorScale_getColorAtPos(this.cppPointer, pos, ucharArray.byteOffset);
  var ret = tulip.Color(ucharArray[0], ucharArray[1], ucharArray[2], ucharArray[3]);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};

tulip.ColorScale.prototype.getColorMap = function() {
  checkWrappedCppPointer(this.cppPointer);
  var nbColors = _ColorScale_numberOfColors(this.cppPointer);
  var floatArray = _allocArrayInEmHeap(Float32Array, nbColors);
  var ucharArray = _allocArrayInEmHeap(Uint8Array, nbColors*4);
  _ColorScale_getOffsets(this.cppPointer, floatArray.byteOffset);
  _ColorScale_getColors(this.cppPointer, ucharArray.byteOffset);
  var ret = {};
  for (var i = 0 ; i < nbColors ; ++i) {
    ret[floatArray[i]] = tulip.Color(ucharArray[4*i], ucharArray[4*i+1], ucharArray[4*i+2], ucharArray[4*i+3]);
  }
  _freeArrayInEmHeap(floatArray);
  _freeArrayInEmHeap(ucharArray);
  return ret;
};

// ==================================================================================================================

var _holdObservers = Module.cwrap('holdObservers', null, []);
var _unholdObservers = Module.cwrap('unholdObservers', null, []);

tulip.holdObservers = function() {
  _holdObservers();
};

tulip.unholdObservers = function() {
  _unholdObservers();
};

// ==================================================================================================================

tulip.NodeShape = {
  Circle : 14,
  Cone : 3,
  Cross : 8,
  Cube : 0,
  CubeOutlined : 1,
  CubeOutlinedTransparent : 9,
  Cylinder : 6,
  Diamond : 5,
  HalfCylinder : 10,
  Hexagon : 13,
  Pentagon : 12,
  Ring : 15,
  RoundedBox : 18,
  Sphere : 2,
  Square : 4,
  Triangle : 11,
  Star : 19,
  FontAwesomeIcon : 20
};

tulip.EdgeShape  = {
  Polyline : 0,
  BezierCurve : 4,
  CatmullRomCurve : 8,
  CubicBSplineCurve : 16
};

tulip.EdgeExtremityShape = {
  None : 1,
  Arrow : 50,
  Circle : 14,
  Cone : 3,
  Cross : 8,
  Cube : 0,
  CubeOutlinedTransparent : 9,
  Cylinder : 6,
  Diamond : 5,
  GlowSphere : 16,
  Hexagon : 13,
  Pentagon : 12,
  Ring : 15,
  Sphere : 2 ,
  Square : 4,
  Star : 19,
  FontAwesomeIcon : 20
};

tulip.FontAwesomeIcon = {
  Adjust : "adjust",
  Adn : "adn",
  AlignCenter : "align-center",
  AlignJustify : "align-justify",
  AlignLeft : "align-left",
  AlignRight : "align-right",
  Ambulance : "ambulance",
  Anchor : "anchor",
  Android : "android",
  Angellist : "angellist",
  AngleDoubleDown : "angle-double-down",
  AngleDoubleLeft : "angle-double-left",
  AngleDoubleRight : "angle-double-right",
  AngleDoubleUp : "angle-double-up",
  AngleDown : "angle-down",
  AngleLeft : "angle-left",
  AngleRight : "angle-right",
  AngleUp : "angle-up",
  Apple : "apple",
  Archive : "archive",
  AreaChart : "area-chart",
  ArrowCircleDown : "arrow-circle-down",
  ArrowCircleLeft : "arrow-circle-left",
  ArrowCircleODown : "arrow-circle-o-down",
  ArrowCircleOLeft : "arrow-circle-o-left",
  ArrowCircleORight : "arrow-circle-o-right",
  ArrowCircleOUp : "arrow-circle-o-up",
  ArrowCircleRight : "arrow-circle-right",
  ArrowCircleUp : "arrow-circle-up",
  ArrowDown : "arrow-down",
  ArrowLeft : "arrow-left",
  ArrowRight : "arrow-right",
  ArrowUp : "arrow-up",
  Arrows : "arrows",
  ArrowsAlt : "arrows-alt",
  ArrowsH : "arrows-h",
  ArrowsV : "arrows-v",
  Asterisk : "asterisk",
  At : "at",
  Automobile : "automobile",
  Backward : "backward",
  Ban : "ban",
  Bank : "bank",
  BarChart : "bar-chart",
  BarChartO : "bar-chart-o",
  Barcode : "barcode",
  Bars : "bars",
  Bed : "bed",
  Beer : "beer",
  Behance : "behance",
  BehanceSquare : "behance-square",
  Bell : "bell",
  BellO : "bell-o",
  BellSlash : "bell-slash",
  BellSlashO : "bell-slash-o",
  Bicycle : "bicycle",
  Binoculars : "binoculars",
  BirthdayCake : "birthday-cake",
  Bitbucket : "bitbucket",
  BitbucketSquare : "bitbucket-square",
  Bitcoin : "bitcoin",
  Bold : "bold",
  Bolt : "bolt",
  Bomb : "bomb",
  Book : "book",
  Bookmark : "bookmark",
  BookmarkO : "bookmark-o",
  Briefcase : "briefcase",
  Btc : "btc",
  Bug : "bug",
  Building : "building",
  BuildingO : "building-o",
  Bullhorn : "bullhorn",
  Bullseye : "bullseye",
  Bus : "bus",
  Buysellads : "buysellads",
  Cab : "cab",
  Calculator : "calculator",
  Calendar : "calendar",
  CalendarO : "calendar-o",
  Camera : "camera",
  CameraRetro : "camera-retro",
  Car : "car",
  CaretDown : "caret-down",
  CaretLeft : "caret-left",
  CaretRight : "caret-right",
  CaretSquareODown : "caret-square-o-down",
  CaretSquareOLeft : "caret-square-o-left",
  CaretSquareORight : "caret-square-o-right",
  CaretSquareOUp : "caret-square-o-up",
  CaretUp : "caret-up",
  CartArrowDown : "cart-arrow-down",
  CartPlus : "cart-plus",
  Cc : "cc",
  CcAmex : "cc-amex",
  CcDiscover : "cc-discover",
  CcMastercard : "cc-mastercard",
  CcPaypal : "cc-paypal",
  CcStripe : "cc-stripe",
  CcVisa : "cc-visa",
  Certificate : "certificate",
  Chain : "chain",
  ChainBroken : "chain-broken",
  Check : "check",
  CheckCircle : "check-circle",
  CheckCircleO : "check-circle-o",
  CheckSquare : "check-square",
  CheckSquareO : "check-square-o",
  ChevronCircleDown : "chevron-circle-down",
  ChevronCircleLeft : "chevron-circle-left",
  ChevronCircleRight : "chevron-circle-right",
  ChevronCircleUp : "chevron-circle-up",
  ChevronDown : "chevron-down",
  ChevronLeft : "chevron-left",
  ChevronRight : "chevron-right",
  ChevronUp : "chevron-up",
  Child : "child",
  Circle : "circle",
  CircleO : "circle-o",
  CircleONotch : "circle-o-notch",
  CircleThin : "circle-thin",
  Clipboard : "clipboard",
  ClockO : "clock-o",
  Close : "close",
  Cloud : "cloud",
  CloudDownload : "cloud-download",
  CloudUpload : "cloud-upload",
  Cny : "cny",
  Code : "code",
  CodeFork : "code-fork",
  Codepen : "codepen",
  Coffee : "coffee",
  Cog : "cog",
  Cogs : "cogs",
  Columns : "columns",
  Comment : "comment",
  CommentO : "comment-o",
  Comments : "comments",
  CommentsO : "comments-o",
  Compass : "compass",
  Compress : "compress",
  Connectdevelop : "connectdevelop",
  Copy : "copy",
  Copyright : "copyright",
  CreditCard : "credit-card",
  Crop : "crop",
  Crosshairs : "crosshairs",
  Css3 : "css3",
  Cube : "cube",
  Cubes : "cubes",
  Cut : "cut",
  Cutlery : "cutlery",
  Dashboard : "dashboard",
  Dashcube : "dashcube",
  Database : "database",
  Dedent : "dedent",
  Delicious : "delicious",
  Desktop : "desktop",
  Deviantart : "deviantart",
  Diamond : "diamond",
  Digg : "digg",
  Dollar : "dollar",
  DotCircleO : "dot-circle-o",
  Download : "download",
  Dribbble : "dribbble",
  Dropbox : "dropbox",
  Drupal : "drupal",
  Edit : "edit",
  Eject : "eject",
  EllipsisH : "ellipsis-h",
  EllipsisV : "ellipsis-v",
  Empire : "empire",
  Envelope : "envelope",
  EnvelopeO : "envelope-o",
  EnvelopeSquare : "envelope-square",
  Eraser : "eraser",
  Eur : "eur",
  Euro : "euro",
  Exchange : "exchange",
  Exclamation : "exclamation",
  ExclamationCircle : "exclamation-circle",
  ExclamationTriangle : "exclamation-triangle",
  Expand : "expand",
  ExternalLink : "external-link",
  ExternalLinkSquare : "external-link-square",
  Eye : "eye",
  EyeSlash : "eye-slash",
  Eyedropper : "eyedropper",
  Facebook : "facebook",
  FacebookF : "facebook-f",
  FacebookOfficial : "facebook-official",
  FacebookSquare : "facebook-square",
  FastBackward : "fast-backward",
  FastForward : "fast-forward",
  Fax : "fax",
  Female : "female",
  FighterJet : "fighter-jet",
  File : "file",
  FileArchiveO : "file-archive-o",
  FileAudioO : "file-audio-o",
  FileCodeO : "file-code-o",
  FileExcelO : "file-excel-o",
  FileImageO : "file-image-o",
  FileMovieO : "file-movie-o",
  FileO : "file-o",
  FilePdfO : "file-pdf-o",
  FilePhotoO : "file-photo-o",
  FilePictureO : "file-picture-o",
  FilePowerpointO : "file-powerpoint-o",
  FileSoundO : "file-sound-o",
  FileText : "file-text",
  FileTextO : "file-text-o",
  FileVideoO : "file-video-o",
  FileWordO : "file-word-o",
  FileZipO : "file-zip-o",
  FilesO : "files-o",
  Film : "film",
  Filter : "filter",
  Fire : "fire",
  FireExtinguisher : "fire-extinguisher",
  Flag : "flag",
  FlagCheckered : "flag-checkered",
  FlagO : "flag-o",
  Flash : "flash",
  Flask : "flask",
  Flickr : "flickr",
  FloppyO : "floppy-o",
  Folder : "folder",
  FolderO : "folder-o",
  FolderOpen : "folder-open",
  FolderOpenO : "folder-open-o",
  Font : "font",
  Forumbee : "forumbee",
  Forward : "forward",
  Foursquare : "foursquare",
  FrownO : "frown-o",
  FutbolO : "futbol-o",
  Gamepad : "gamepad",
  Gavel : "gavel",
  Gbp : "gbp",
  Ge : "ge",
  Gear : "gear",
  Gears : "gears",
  Genderless : "genderless",
  Gift : "gift",
  Git : "git",
  GitSquare : "git-square",
  Github : "github",
  GithubAlt : "github-alt",
  GithubSquare : "github-square",
  Gittip : "gittip",
  Glass : "glass",
  Globe : "globe",
  Google : "google",
  GooglePlus : "google-plus",
  GooglePlusSquare : "google-plus-square",
  GoogleWallet : "google-wallet",
  GraduationCap : "graduation-cap",
  Gratipay : "gratipay",
  Group : "group",
  HSquare : "h-square",
  HackerNews : "hacker-news",
  HandODown : "hand-o-down",
  HandOLeft : "hand-o-left",
  HandORight : "hand-o-right",
  HandOUp : "hand-o-up",
  HddO : "hdd-o",
  Header : "header",
  Headphones : "headphones",
  Heart : "heart",
  HeartO : "heart-o",
  Heartbeat : "heartbeat",
  History : "history",
  Home : "home",
  HospitalO : "hospital-o",
  Hotel : "hotel",
  Html5 : "html5",
  Ils : "ils",
  Image : "image",
  Inbox : "inbox",
  Indent : "indent",
  Info : "info",
  InfoCircle : "info-circle",
  Inr : "inr",
  Instagram : "instagram",
  Institution : "institution",
  Ioxhost : "ioxhost",
  Italic : "italic",
  Joomla : "joomla",
  Jpy : "jpy",
  Jsfiddle : "jsfiddle",
  Key : "key",
  KeyboardO : "keyboard-o",
  Krw : "krw",
  Language : "language",
  Laptop : "laptop",
  Lastfm : "lastfm",
  LastfmSquare : "lastfm-square",
  Leaf : "leaf",
  Leanpub : "leanpub",
  Legal : "legal",
  LemonO : "lemon-o",
  LevelDown : "level-down",
  LevelUp : "level-up",
  LifeBouy : "life-bouy",
  LifeBuoy : "life-buoy",
  LifeRing : "life-ring",
  LifeSaver : "life-saver",
  LightbulbO : "lightbulb-o",
  LineChart : "line-chart",
  Link : "link",
  Linkedin : "linkedin",
  LinkedinSquare : "linkedin-square",
  Linux : "linux",
  List : "list",
  ListAlt : "list-alt",
  ListOl : "list-ol",
  ListUl : "list-ul",
  LocationArrow : "location-arrow",
  Lock : "lock",
  LongArrowDown : "long-arrow-down",
  LongArrowLeft : "long-arrow-left",
  LongArrowRight : "long-arrow-right",
  LongArrowUp : "long-arrow-up",
  Magic : "magic",
  Magnet : "magnet",
  MailForward : "mail-forward",
  MailReply : "mail-reply",
  MailReplyAll : "mail-reply-all",
  Male : "male",
  MapMarker : "map-marker",
  Mars : "mars",
  MarsDouble : "mars-double",
  MarsStroke : "mars-stroke",
  MarsStrokeH : "mars-stroke-h",
  MarsStrokeV : "mars-stroke-v",
  Maxcdn : "maxcdn",
  Meanpath : "meanpath",
  Medium : "medium",
  Medkit : "medkit",
  MehO : "meh-o",
  Mercury : "mercury",
  Microphone : "microphone",
  MicrophoneSlash : "microphone-slash",
  Minus : "minus",
  MinusCircle : "minus-circle",
  MinusSquare : "minus-square",
  MinusSquareO : "minus-square-o",
  Mobile : "mobile",
  MobilePhone : "mobile-phone",
  Money : "money",
  MoonO : "moon-o",
  MortarBoard : "mortar-board",
  Motorcycle : "motorcycle",
  Music : "music",
  Navicon : "navicon",
  Neuter : "neuter",
  NewspaperO : "newspaper-o",
  Openid : "openid",
  Outdent : "outdent",
  Pagelines : "pagelines",
  PaintBrush : "paint-brush",
  PaperPlane : "paper-plane",
  PaperPlaneO : "paper-plane-o",
  Paperclip : "paperclip",
  Paragraph : "paragraph",
  Paste : "paste",
  Pause : "pause",
  Paw : "paw",
  Paypal : "paypal",
  Pencil : "pencil",
  PencilSquare : "pencil-square",
  PencilSquareO : "pencil-square-o",
  Phone : "phone",
  PhoneSquare : "phone-square",
  Photo : "photo",
  PictureO : "picture-o",
  PieChart : "pie-chart",
  PiedPiper : "pied-piper",
  PiedPiperAlt : "pied-piper-alt",
  Pinterest : "pinterest",
  PinterestP : "pinterest-p",
  PinterestSquare : "pinterest-square",
  Plane : "plane",
  Play : "play",
  PlayCircle : "play-circle",
  PlayCircleO : "play-circle-o",
  Plug : "plug",
  Plus : "plus",
  PlusCircle : "plus-circle",
  PlusSquare : "plus-square",
  PlusSquareO : "plus-square-o",
  PowerOff : "power-off",
  Print : "print",
  PuzzlePiece : "puzzle-piece",
  Qq : "qq",
  Qrcode : "qrcode",
  Question : "question",
  QuestionCircle : "question-circle",
  QuoteLeft : "quote-left",
  QuoteRight : "quote-right",
  Ra : "ra",
  Random : "random",
  Rebel : "rebel",
  Recycle : "recycle",
  Reddit : "reddit",
  RedditSquare : "reddit-square",
  Refresh : "refresh",
  Remove : "remove",
  Renren : "renren",
  Reorder : "reorder",
  Repeat : "repeat",
  Reply : "reply",
  ReplyAll : "reply-all",
  Retweet : "retweet",
  Rmb : "rmb",
  Road : "road",
  Rocket : "rocket",
  RotateLeft : "rotate-left",
  RotateRight : "rotate-right",
  Rouble : "rouble",
  Rss : "rss",
  RssSquare : "rss-square",
  Rub : "rub",
  Ruble : "ruble",
  Rupee : "rupee",
  Save : "save",
  Scissors : "scissors",
  Search : "search",
  SearchMinus : "search-minus",
  SearchPlus : "search-plus",
  Sellsy : "sellsy",
  Send : "send",
  SendO : "send-o",
  Server : "server",
  Share : "share",
  ShareAlt : "share-alt",
  ShareAltSquare : "share-alt-square",
  ShareSquare : "share-square",
  ShareSquareO : "share-square-o",
  Shekel : "shekel",
  Sheqel : "sheqel",
  Shield : "shield",
  Ship : "ship",
  Shirtsinbulk : "shirtsinbulk",
  ShoppingCart : "shopping-cart",
  SignIn : "sign-in",
  SignOut : "sign-out",
  Signal : "signal",
  Simplybuilt : "simplybuilt",
  Sitemap : "sitemap",
  Skyatlas : "skyatlas",
  Skype : "skype",
  Slack : "slack",
  Sliders : "sliders",
  Slideshare : "slideshare",
  SmileO : "smile-o",
  SoccerBallO : "soccer-ball-o",
  Sort : "sort",
  SortAlphaAsc : "sort-alpha-asc",
  SortAlphaDesc : "sort-alpha-desc",
  SortAmountAsc : "sort-amount-asc",
  SortAmountDesc : "sort-amount-desc",
  SortAsc : "sort-asc",
  SortDesc : "sort-desc",
  SortDown : "sort-down",
  SortNumericAsc : "sort-numeric-asc",
  SortNumericDesc : "sort-numeric-desc",
  SortUp : "sort-up",
  Soundcloud : "soundcloud",
  SpaceShuttle : "space-shuttle",
  Spinner : "spinner",
  Spoon : "spoon",
  Spotify : "spotify",
  Square : "square",
  SquareO : "square-o",
  StackExchange : "stack-exchange",
  StackOverflow : "stack-overflow",
  Star : "star",
  StarHalf : "star-half",
  StarHalfEmpty : "star-half-empty",
  StarHalfFull : "star-half-full",
  StarHalfO : "star-half-o",
  StarO : "star-o",
  Steam : "steam",
  SteamSquare : "steam-square",
  StepBackward : "step-backward",
  StepForward : "step-forward",
  Stethoscope : "stethoscope",
  Stop : "stop",
  StreetView : "street-view",
  Strikethrough : "strikethrough",
  Stumbleupon : "stumbleupon",
  StumbleuponCircle : "stumbleupon-circle",
  Subscript : "subscript",
  Subway : "subway",
  Suitcase : "suitcase",
  SunO : "sun-o",
  Superscript : "superscript",
  Support : "support",
  Table : "table",
  Tablet : "tablet",
  Tachometer : "tachometer",
  Tag : "tag",
  Tags : "tags",
  Tasks : "tasks",
  Taxi : "taxi",
  TencentWeibo : "tencent-weibo",
  Terminal : "terminal",
  TextHeight : "text-height",
  TextWidth : "text-width",
  Th : "th",
  ThLarge : "th-large",
  ThList : "th-list",
  ThumbTack : "thumb-tack",
  ThumbsDown : "thumbs-down",
  ThumbsODown : "thumbs-o-down",
  ThumbsOUp : "thumbs-o-up",
  ThumbsUp : "thumbs-up",
  Ticket : "ticket",
  Times : "times",
  TimesCircle : "times-circle",
  TimesCircleO : "times-circle-o",
  Tint : "tint",
  ToggleDown : "toggle-down",
  ToggleLeft : "toggle-left",
  ToggleOff : "toggle-off",
  ToggleOn : "toggle-on",
  ToggleRight : "toggle-right",
  ToggleUp : "toggle-up",
  Train : "train",
  Transgender : "transgender",
  TransgenderAlt : "transgender-alt",
  Trash : "trash",
  TrashO : "trash-o",
  Tree : "tree",
  Trello : "trello",
  Trophy : "trophy",
  Truck : "truck",
  Try : "try",
  Tty : "tty",
  Tumblr : "tumblr",
  TumblrSquare : "tumblr-square",
  TurkishLira : "turkish-lira",
  Twitch : "twitch",
  Twitter : "twitter",
  TwitterSquare : "twitter-square",
  Umbrella : "umbrella",
  Underline : "underline",
  Undo : "undo",
  University : "university",
  Unlink : "unlink",
  Unlock : "unlock",
  UnlockAlt : "unlock-alt",
  Unsorted : "unsorted",
  Upload : "upload",
  Usd : "usd",
  User : "user",
  UserMd : "user-md",
  UserPlus : "user-plus",
  UserSecret : "user-secret",
  UserTimes : "user-times",
  Users : "users",
  Venus : "venus",
  VenusDouble : "venus-double",
  VenusMars : "venus-mars",
  Viacoin : "viacoin",
  VideoCamera : "video-camera",
  VimeoSquare : "vimeo-square",
  Vine : "vine",
  Vk : "vk",
  VolumeDown : "volume-down",
  VolumeOff : "volume-off",
  VolumeUp : "volume-up",
  Warning : "warning",
  Wechat : "wechat",
  Weibo : "weibo",
  Weixin : "weixin",
  Whatsapp : "whatsapp",
  Wheelchair : "wheelchair",
  Wifi : "wifi",
  Windows : "windows",
  Won : "won",
  Wordpress : "wordpress",
  Wrench : "wrench",
  Xing : "xing",
  XingSquare : "xing-square",
  Yahoo : "yahoo",
  Yelp : "yelp",
  Yen : "yen",
  Youtube : "youtube",
  YoutubePlay : "youtube-play",
  YoutubeSquare : "youtube-square"
};

// ==================================================================================================================

tulip.GraphEventType = {
  TLP_ADD_NODE : 0,
  TLP_DEL_NODE : 1,
  TLP_ADD_EDGE : 2,
  TLP_DEL_EDGE : 3,
  TLP_REVERSE_EDGE : 4,
  TLP_BEFORE_SET_ENDS : 5,
  TLP_AFTER_SET_ENDS : 6,
  TLP_ADD_NODES : 7,
  TLP_ADD_EDGES : 8,
  TLP_BEFORE_ADD_DESCENDANTGRAPH : 9,
  TLP_AFTER_ADD_DESCENDANTGRAPH : 10,
  TLP_BEFORE_DEL_DESCENDANTGRAPH : 11,
  TLP_AFTER_DEL_DESCENDANTGRAPH : 12,
  TLP_BEFORE_ADD_SUBGRAPH : 13,
  TLP_AFTER_ADD_SUBGRAPH : 14,
  TLP_BEFORE_DEL_SUBGRAPH : 15,
  TLP_AFTER_DEL_SUBGRAPH : 16,
  TLP_ADD_LOCAL_PROPERTY : 17,
  TLP_BEFORE_DEL_LOCAL_PROPERTY : 18,
  TLP_AFTER_DEL_LOCAL_PROPERTY : 19,
  TLP_ADD_INHERITED_PROPERTY : 20,
  TLP_BEFORE_DEL_INHERITED_PROPERTY : 21,
  TLP_AFTER_DEL_INHERITED_PROPERTY : 22,
  TLP_BEFORE_RENAME_LOCAL_PROPERTY : 23,
  TLP_AFTER_RENAME_LOCAL_PROPERTY : 24,
  TLP_BEFORE_SET_ATTRIBUTE : 25,
  TLP_AFTER_SET_ATTRIBUTE : 26,
  TLP_REMOVE_ATTRIBUTE : 27,
  TLP_BEFORE_ADD_LOCAL_PROPERTY : 28,
  TLP_BEFORE_ADD_INHERITED_PROPERTY : 29
};

tulip.GraphEvent = function(graph, eventType) {
  var newObject = createObject(tulip.GraphEvent, this);
  newObject.type = "GraphEvent";
  newObject.graph = graph;
  newObject.eventType = eventType;
  newObject.node = null;
  newObject.edge = null;
  newObject.name = null;
  newObject.nodes = null;
  newObject.edges = null;
  return newObject;
};

// ==================================================================================================================

tulip.PropertyEventType = {
  TLP_BEFORE_SET_NODE_VALUE: 0,
  TLP_AFTER_SET_NODE_VALUE: 1,
  TLP_BEFORE_SET_ALL_NODE_VALUE : 2,
  TLP_AFTER_SET_ALL_NODE_VALUE : 3,
  TLP_BEFORE_SET_ALL_EDGE_VALUE : 4,
  TLP_AFTER_SET_ALL_EDGE_VALUE : 5,
  TLP_BEFORE_SET_EDGE_VALUE : 6,
  TLP_AFTER_SET_EDGE_VALUE : 7
};

tulip.PropertyEvent = function(property, eventType) {
  var newObject = createObject(tulip.PropertyEvent, this);
  newObject.type = "PropertyEvent";
  newObject.property = property;
  newObject.eventType = eventType;
  newObject.node = null;
  newObject.edge = null;
  return newObject;
};

// ==================================================================================================================

var _tulipListeners = {};

tulip.addListener = function(type, listener) {
  if (typeof _tulipListeners[type] == "undefined"){
    _tulipListeners[type] = [];
  }
  _tulipListeners[type].push(listener);
};

tulip.fire = function(event) {
  if (typeof event == "string") {
    event = { type: event };
  }
  if (_tulipListeners[event.type] instanceof Array) {
    var listeners = _tulipListeners[event.type];
    for (var i = 0, len = listeners.length ; i < len ; i++) {
      listeners[i].call(this, event);
    }
  }
};

tulip.removeListener = function(type, listener) {
  if (_tulipListeners[type] instanceof Array) {
    var listeners = _tulipListeners[type];
    for (var i = 0, len = listeners.length ; i < len ; i++) {
      if (listeners[i] === listener) {
        listeners.splice(i, 1);
        break;
      }
    }
  }
};

// ==================================================================================================================

if (workerMode) {

  var graphObject = {};
  var propertiesNames = {};
  var curNodeId = {};
  var curEdgeId = {};
  var propertiesFilter = {};
  var updateMode = {};
  var subGraphsData = {};
  var curSubGraphIdx = {};
  var nodesData = {};
  var edgesData = {};
  var graphs = {}

  var maxBatchSize = 300;
  var graphElementsBatchSize = {};

  function clamp(val, min, max) {
    return Math.max(Math.min(val, max), min);
  }

  function getNodeObject(graphId, nodeId) {
    var graph = graphObject[graphId];
    var nodeDataObj = {};
    nodeDataObj.nodeId = nodeId;
    nodeDataObj.properties = {};
    for (var j = 0 ; j < propertiesNames[graphId].length ; ++j) {
      var propertyName = propertiesNames[graphId][j];
      if (!propertiesFilter[graphId] || propertiesFilter[graphId].indexOf(propertyName) != -1) {
        var propertyData = graph.properties[propertyName];
        if (propertyData.nodesValues != undefined) {
          if (propertyData.nodesValues[nodeId] != undefined) {
            nodeDataObj.properties[propertyName] = propertyData.nodesValues[nodeId];
          } else {
            nodeDataObj.properties[propertyName] = propertyData.nodeDefault;
          }
        } else {
          nodeDataObj.properties[propertyName] = propertyData.nodeDefault;
        }
      }
    }
    return nodeDataObj;
  }

  function getEdgeObject(graphId, edgeId) {
    var graph = graphObject[graphId];
    var edgeDataObj = {};
    edgeDataObj.edgeId = edgeId;
    edgeDataObj.srcNodeId = graph.edges[edgeId][0];
    edgeDataObj.tgtNodeId = graph.edges[edgeId][1];
    edgeDataObj.properties = {};
    for (var j = 0 ; j < propertiesNames[graphId].length ; ++j) {
      var propertyName = propertiesNames[graphId][j];
      if (!propertiesFilter[graphId] || propertiesFilter[graphId].indexOf(propertyName) != -1) {
        var propertyData = graph.properties[propertyName];
        if (propertyData.edgesValues != undefined) {
          if (propertyData.edgesValues[edgeId] != undefined) {
            edgeDataObj.properties[propertyName] = propertyData.edgesValues[edgeId];
          } else {
            edgeDataObj.properties[propertyName] = propertyData.edgeDefault;
          }
        } else {
          edgeDataObj.properties[propertyName] = propertyData.edgeDefault;
        }
      }
    }
    return edgeDataObj;
  }

  function fetchNextNodesData(graphId) {
    curNodeId[graphId] = curNodeId[graphId] + 1;
    nodesData[graphId] = [];
    if (curNodeId[graphId] == graphObject[graphId].nodesNumber) {
      return;
    }
    var boundNodeId = curNodeId[graphId] + graphElementsBatchSize[graphId];
    if (boundNodeId >= graphObject[graphId].nodesNumber) {
      boundNodeId = graphObject[graphId].nodesNumber;
    }
    for (var i = curNodeId[graphId] ; i < boundNodeId ; ++i) {
      nodesData[graphId].push(getNodeObject(graphId, i));
    }
    curNodeId[graphId] = boundNodeId - 1;
  }

  function fetchNextEdgesData(graphId) {
    curEdgeId[graphId] = curEdgeId[graphId] + 1;
    edgesData[graphId] = [];
    if (curEdgeId[graphId] == graphObject[graphId].edgesNumber) {
      return;
    }
    var boundEdgeId = curEdgeId[graphId] + graphElementsBatchSize[graphId];
    if (boundEdgeId >= graphObject[graphId].edgesNumber) {
      boundEdgeId = graphObject[graphId].edgesNumber;
    }
    for (var i = curEdgeId[graphId] ; i < boundEdgeId ; ++i) {
      edgesData[graphId].push(getEdgeObject(graphId, i));
    }
    curEdgeId[graphId] = boundEdgeId - 1;
  }

  var _setPluginProgressGraphId = Module.cwrap('setPluginProgressGraphId', null, ['number']);

  function sendGraphData(graphId) {
    curNodeId[graphId] = 0;
    curEdgeId[graphId] = 0;
    self.postMessage({eventType : 'addNodes', graphId : graphId,
                       nodesJson : JSON.stringify([getNodeObject(graphId, 0)]),
                       lastNodeId : 0});
  }

  function prepareSubGraphsData(graphId, graphObj) {
    for (var i = 0 ; i < graphObj.subgraphs.length ; ++i) {
      var subGraphObj = graphObj.subgraphs[i];
      var subGraphDataObj = {};
      subGraphDataObj.parentGraphId = graphObj.graphID;
      subGraphDataObj.subGraphId = subGraphObj.graphID;
      subGraphDataObj.nodesIds = "(";
      for (var j = 0 ; j < subGraphObj.nodesIDs.length ; ++j) {
        if (typeOf(subGraphObj.nodesIDs[j]) == "number") {
          subGraphDataObj.nodesIds += subGraphObj.nodesIDs[j].toString();
        } else {
          for (var k = subGraphObj.nodesIDs[j][0] ; k <= subGraphObj.nodesIDs[j][1] ; ++k) {
            subGraphDataObj.nodesIds += k.toString();
            if (k != subGraphObj.nodesIDs[j][1]) {
              subGraphDataObj.nodesIds += ", ";
            }
          }
        }
        if (j != subGraphObj.nodesIDs.length - 1) {
          subGraphDataObj.nodesIds += ", ";
        }
      }
      subGraphDataObj.nodesIds += ")";
      subGraphDataObj.edgesIds = "(";
      for (var j = 0 ; j < subGraphObj.edgesIDs.length ; ++j) {
        if (typeOf(subGraphObj.edgesIDs[j]) == "number") {
          subGraphDataObj.edgesIds += subGraphObj.edgesIDs[j].toString();
        } else {
          for (var k = subGraphObj.edgesIDs[j][0] ; k <= subGraphObj.edgesIDs[j][1] ; ++k) {
            subGraphDataObj.edgesIds += k.toString();
            if (k != subGraphObj.edgesIDs[j][1]) {
              subGraphDataObj.edgesIds += ", ";
            }
          }
        }
        if (j != subGraphObj.edgesIDs.length - 1) {
          subGraphDataObj.edgesIds += ", ";
        }
      }
      subGraphDataObj.edgesIds += ")";
      subGraphDataObj.attributes = JSON.stringify(subGraphObj.attributes);
      subGraphsData[graphId].push(subGraphDataObj);
      prepareSubGraphsData(graphId, subGraphObj);
    }
  }

  function getGraphData(graphId, graphDataLoadedCallback) {
    tulip.sendProgressComment(graphId, "Exporting graph to JSON format ...");
    tulip.sendProgressValue(graphId, -1);
    setTimeout(function() {
      var jsonGraphStr = graphs[graphId].toJSON();
      var jsonGraph = JSON.parse(jsonGraphStr);
      graphObject[graphId] = jsonGraph.graph;
      subGraphsData[graphId] = [];
      prepareSubGraphsData(graphId, graphObject[graphId]);
      graphElementsBatchSize[graphId] = clamp(graphObject[graphId].nodesNumber, 1, maxBatchSize) | 0;
      var propertiesNamesTmp = Object.keys(graphObject[graphId].properties);
      propertiesNames[graphId] = [];
      for (var i = 0 ; i < propertiesNamesTmp.length ; ++i) {
        propertiesNames[graphId].push(propertiesNamesTmp[i]);
      }
      graphDataLoadedCallback();
    }, 0);
  }

  function loadGraph(graphId, graphFilePath, sendData) {
    _setPluginProgressGraphId(graphId);
    graphs[graphId] = tulip.loadGraph(graphFilePath, sendData);

    if (!sendData) return;
    getGraphData(graphId, function() {
      updateMode[graphId] = false;
      self.postMessage({eventType : 'startGraphData',
                         graphId : graphId,
                         numberOfNodes : graphObject[graphId].nodesNumber,
                         numberOfEdges : graphObject[graphId].edgesNumber,
                         graphAttributes : JSON.stringify(graphObject[graphId].attributes)
                       });
      var propertiesData = {};
      for (var i = 0 ; i < propertiesNames[graphId].length ; ++i) {
        propertiesData[propertiesNames[graphId][i]] = {};
        propertiesData[propertiesNames[graphId][i]].type = graphObject[graphId].properties[propertiesNames[graphId][i]].type;
        propertiesData[propertiesNames[graphId][i]].nodeDefault = graphObject[graphId].properties[propertiesNames[graphId][i]].nodeDefault;
        propertiesData[propertiesNames[graphId][i]].edgeDefault = graphObject[graphId].properties[propertiesNames[graphId][i]].edgeDefault;
      }
      self.postMessage({eventType : 'createGraphProperties', graphId : graphId, properties : propertiesData});
      sendGraphData(graphId);
    });
  }

  self.addEventListener('message', function(e) {
    var data = e.data;
    if (!data) return;
    switch (data.eventType) {
    case 'loadGraph':
      if (!data.graphFileData) {
        var graphReq = new XMLHttpRequest();
        graphReq.open("GET", data.graphFile, true);
        graphReq.responseType = "arraybuffer";
        graphReq.onload = function (oEvent) {
          var arrayBuffer = graphReq.response;
          var file = FS.findObject(data.graphFile);
          if (!file) {
            var paths = data.graphFile.split('/');
            var filePath = "/";
            for (var i = 0; i < paths.length - 1; ++i) {
              filePath += paths[i];
              filePath += "/";
            }
            FS.createPath('/', filePath, true, true);
            FS.createFile('/', data.graphFile, {}, true, true);
          }
          FS.writeFile(data.graphFile, new Uint8Array(arrayBuffer), {'encoding' : 'binary'});
          var graphToDestroy = null;
          if (data.graphId in graphs) {
            graphToDestroy = graphs[data.graphId];
          }
          loadGraph(data.graphId, data.graphFile, data.sendDataBack);
          if (graphToDestroy) graphToDestroy.destroy();
        };
        graphReq.send(null);
      } else {
        var file = FS.findObject('/' + data.graphFile);
        if (!file) {
          var paths = data.graphFile.split('/');
          var filePath = "/";
          for (var i = 0; i < paths.length - 1; ++i) {
            filePath += paths[i];
            filePath += "/";
          }
          FS.createPath('/', filePath, true, true);
          FS.createFile('/', data.graphFile, {}, true, true);
        }
        FS.writeFile('/' + data.graphFile, new Uint8Array(data.graphFileData), {'encoding' : 'binary'});
        var graphToDestroy = null;
        if (data.graphId in graphs) {
          graphToDestroy = graphs[data.graphId];
        }
        loadGraph(data.graphId, data.graphFile, data.sendDataBack);
        if (graphToDestroy) graphToDestroy.destroy();
      }
      break;
    case 'sendNextNodes':
      fetchNextNodesData(data.graphId);
      if (nodesData[data.graphId].length > 0) {
        self.postMessage({eventType : 'addNodes', graphId : data.graphId,
                           nodesJson : JSON.stringify(nodesData[data.graphId]),
                           lastNodeId : nodesData[data.graphId][nodesData[data.graphId].length - 1].nodeId});
      } else {
        graphElementsBatchSize[data.graphId] = clamp(graphObject[data.graphId].edgesNumber, 1, maxBatchSize) | 0;
        self.postMessage({eventType : 'addEdges', graphId : data.graphId,
                           edgesJson : JSON.stringify([getEdgeObject(data.graphId, 0)]),
                           lastEdgeId : 0});
      }
      break;
    case 'sendNextEdges':
      fetchNextEdgesData(data.graphId);
      if (edgesData[data.graphId].length == 0) {
        if (!updateMode[data.graphId]) {
          if (subGraphsData[data.graphId].length > 0) {
            curSubGraphIdx[data.graphId] = 0;
            self.postMessage({eventType : 'addSubGraph', graphId : data.graphId, subGraphData : subGraphsData[data.graphId][0]});
          } else {
            self.postMessage({eventType : 'endGraphData', graphId : data.graphId});
          }
        } else {
          self.postMessage({eventType : 'endGraphUpdate', graphId : data.graphId});
        }
        propertiesFilter[data.graphId] = null;
        return;
      }
      self.postMessage({eventType : 'addEdges', graphId : data.graphId,
                         edgesJson : JSON.stringify(edgesData[data.graphId]),
                         lastEdgeId : edgesData[data.graphId][edgesData[data.graphId].length - 1].edgeId});
      break;
    case 'sendNextSubGraph':
      curSubGraphIdx[data.graphId] = curSubGraphIdx[data.graphId] + 1;
      if (curSubGraphIdx[data.graphId] < subGraphsData[data.graphId].length) {
        self.postMessage({eventType : 'addSubGraph', graphId : data.graphId, subGraphData : subGraphsData[data.graphId][curSubGraphIdx[data.graphId]]});
      } else {
        if (!updateMode[data.graphId]) {
          self.postMessage({eventType : 'endGraphData', graphId : data.graphId});
        } else {
          self.postMessage({eventType : 'endGraphUpdate', graphId : data.graphId});
        }
      }
      break;
    case 'algorithm' :
      _setPluginProgressGraphId(data.graphId);
      var algoSucceed = graphs[data.graphId].applyAlgorithm(data.algorithmName, JSON.parse(data.parameters), true);
      propertiesFilter[data.graphId] = null;
      updateMode[data.graphId] = true;
      getGraphData(data.graphId, function() {
        self.postMessage({eventType : 'startGraphUpdate',
                           graphId : data.graphId,
                           algoSucceed : algoSucceed,
                           numberOfNodes : graphObject[data.graphId].nodesNumber,
                           numberOfEdges : graphObject[data.graphId].edgesNumber});
        sendGraphData(data.graphId);
      });
      break;
    case 'propertyAlgorithm' :
      _setPluginProgressGraphId(data.graphId);
      var graph = graphs[data.graphId];
      var resultProp = graph.getProperty(data.resultPropertyName);
      var algoSucceed = graph.applyPropertyAlgorithm(data.algorithmName, resultProp, JSON.parse(data.parameters), true);
      propertiesFilter[data.graphId] = [data.resultPropertyName];
      updateMode[data.graphId] = true;
      getGraphData(data.graphId, function() {
        self.postMessage({eventType : 'startGraphUpdate',
                           graphId : data.graphId,
                           algoSucceed : algoSucceed,
                           numberOfNodes : graphObject[data.graphId].nodesNumber,
                           numberOfEdges : graphObject[data.graphId].edgesNumber});
        sendGraphData(data.graphId);
      });
      break;
    case 'executeGraphScript' :
      _setPluginProgressGraphId(data.graphId);
      var graph = graphs[data.graphId];
      var scriptSucceed = true;
      try {
        eval("f = " + data.scriptCode + "; f(graph);");
      } catch (e) {
        console.log("exception caught");
        console.log(e);
        scriptSucceed = false;
      }
      propertiesFilter[data.graphId] = null;
      updateMode[data.graphId] = true;
      getGraphData(data.graphId, function() {
        self.postMessage({eventType : 'startGraphUpdate',
                           graphId : data.graphId,
                           algoSucceed : scriptSucceed,
                           numberOfNodes : graphObject[data.graphId].nodesNumber,
                           numberOfEdges : graphObject[data.graphId].edgesNumber});
        sendGraphData(data.graphId);
      });
      break;
    };
  }, false);

  tulip.sendProgressValue = function(graphId, val) {
    self.postMessage({eventType: 'progressValue', graphId : graphId, value: val});
  };

  tulip.sendProgressComment = function(graphId, text) {
    self.postMessage({eventType: 'progressComment', graphId : graphId, comment: text});
  }

  self.postMessage({eventType : 'tulipWorkerInit'});

} else {

  // ===================================================================================================================

  var _graphLoadedCallback = {};
  var _algorithmFinishedCallback = {};
  var _algorithmSucceed = {};
  var _graphIdToWrapper = {};

  function _createGraphProperties(graphId, properties) {
    var propertiesNames = Object.keys(properties);
    for (var i = 0 ; i < propertiesNames.length ; ++i) {
      var propertyName = propertiesNames[i];
      var propertyData = properties[propertyName];
      var propertyType = propertyData.type;
      var propertyNodeDefaultValue = propertyData.nodeDefault;
      var propertyEdgeDefaultValue = propertyData.edgeDefault;
      _createGraphProperty(graphId, propertyType, propertyName, propertyNodeDefaultValue, propertyEdgeDefaultValue);
    }
  }

  var _fillMetaGraphInfos = Module.cwrap('fillMetaGraphInfos', null, ['number']);
  var _parseGraphAttributesJSONData =  Module.cwrap('parseGraphAttributesJSONData', null, ['number', 'string']);
  var _createGraphProperty = Module.cwrap('createGraphProperty', null, ['number', 'string', 'string', 'string', 'string']);
  var _addSubGraph = Module.cwrap('addSubGraph', null, ['number', 'number', 'number', 'string', 'string', 'string']);
  var _parseNodesJSONData = Module.cwrap('parseNodesJSONData', null, ['number', 'string']);
  var _parseEdgesJSONData = Module.cwrap('parseEdgesJSONData', null, ['number', 'string']);

  if (!tulip.coreBuild) {

    var _centerScene = Module.cwrap('centerScene', null, ['string']);
    var _startGraphViewData = Module.cwrap('startGraphViewData', null, ['string']);
    var _endGraphViewData = Module.cwrap('endGraphViewData', null, ['string']);
    var _startGraphViewUpdate = Module.cwrap('startGraphViewUpdate', null, ['string']);
    var _endGraphViewUpdate = Module.cwrap('endGraphViewUpdate', null, ['string']);
    var _setGraphRenderingDataReady = Module.cwrap('setGraphRenderingDataReady', null, ['string', 'number']);

    var _activateInteractor = Module.cwrap('activateInteractor', null, ['string', 'string']);
    var _desactivateInteractor = Module.cwrap('desactivateInteractor', null, ['string']);

    var _setProgressBarPercent = Module.cwrap('setProgressBarPercent', null, ['string', 'number']);
    var _setProgressBarComment = Module.cwrap('setProgressBarComment', null, ['string', 'string']);

    var _initCanvas = Module.cwrap('initCanvas', null, ['string', 'number', 'number', 'number']);
    var _setCurrentCanvas = Module.cwrap('setCurrentCanvas', null, ['string']);
    var _getCurrentCanvas = Module.cwrap('getCurrentCanvas', 'string', []);
    var _resizeCanvas = Module.cwrap('resizeCanvas', null, ['string', 'number', 'number', 'number']);
    var _draw = Module.cwrap('draw', null, []);
    var _fullScreen = Module.cwrap('fullScreen', null, ['string']);
    var _updateGlScene = Module.cwrap('updateGlScene', null, ['string']);

    var _selectNodes = Module.cwrap('selectNodes', 'number', ['string', 'number', 'number', 'number', 'number']);
    var _getSelectedNodes = Module.cwrap('getSelectedNodes', null, ['number']);
    var _selectEdges = Module.cwrap('selectEdges', 'number', ['string', 'number', 'number', 'number', 'number']);
    var _getSelectedEdges = Module.cwrap('getSelectedEdges', null, ['number']);

    var _setCanvas2dModified = Module.cwrap('setCanvas2dModified', null, ['string']);

    var _nextCanvasId = 0;
    var _graphIdToCanvas = {};
    var _canvasIdToView = {};
    var busyAnimations = {};

    function _startBusyAnimation(canvasId) {

      if (busyAnimations.hasOwnProperty(canvasId) && busyAnimations[canvasId]) {
        return;
      }

      function busyAnimation() {
        if (busyAnimations[canvasId]) {
          _setProgressBarPercent(canvasId, -1);
          _updateGlScene(canvasId);
          Browser.requestAnimationFrame(busyAnimation);
        }
      }

      busyAnimations[canvasId] = true;
      Browser.requestAnimationFrame(busyAnimation);
    }

    function _stopBusyAnimation(canvasId) {
      busyAnimations[canvasId] = false;
    }

    var currentGraphData = null;

  }

  var _tulipWorkerInit = true;

  if (!nodejs) {

    _tulipWorkerInit = false;

    var _tulipWorker = new Worker(scriptPath + scriptName);

    _tulipWorker.addEventListener('message', function (event) {
      var delay = 0;
      var canvasId = "";
      var graphId = null;
      if ('graphId' in event.data) {
        graphId = event.data.graphId;
      }
      if (!tulip.coreBuild) {
        if (graphId) {
          canvasId = _graphIdToCanvas[graphId];
        }
      }
      switch (event.data.eventType) {
      case 'tulipWorkerInit':
        _tulipWorkerInit = true;
        break;
      case 'print':
        console.log(event.data.text);
        break;
      case 'progressValue':
        if (!tulip.coreBuild && canvasId) {
          _stopBusyAnimation(canvasId);
          _setProgressBarPercent(canvasId, event.data.value);
          if (event.data.value >= 0) {
            _draw();
          } else {
            _startBusyAnimation(canvasId);
          }
        }
        break;
      case 'progressComment':
        if (!tulip.coreBuild && canvasId) {
          _setProgressBarComment(canvasId, event.data.comment);
        }
        break;
      case 'startGraphData':
        setTimeout(function() {
          //_graphIdToWrapper[graphId].setEventsActivated(false);
          _parseGraphAttributesJSONData(_graphIdToWrapper[graphId].cppPointer, event.data.graphAttributes);
          if (!tulip.coreBuild && canvasId) {
            _stopBusyAnimation(canvasId);
            _setCurrentCanvas(canvasId);
            _setProgressBarComment(canvasId, "Initializing graph visualization ...");
            _setProgressBarPercent(canvasId, 0);
            _startGraphViewData(canvasId);
            currentGraphData = event.data;
            _draw();
          }
        }, delay);
        break;
      case 'endGraphData':
        //_graphIdToWrapper[graphId].setEventsActivated(true);
        _fillMetaGraphInfos(_graphIdToWrapper[graphId].cppPointer);
        setTimeout(function() {
          if (!tulip.coreBuild && canvasId) {
            _setProgressBarComment(canvasId, "Finalizing rendering data ...");
            _draw();
            _setCurrentCanvas(canvasId);
            _endGraphViewData(canvasId);
            _setGraphRenderingDataReady(canvasId, true);
            _centerScene(canvasId);
          }
          if (graphId in _graphLoadedCallback) {
            _graphLoadedCallback[graphId](_graphIdToWrapper[graphId]);
          }
        }, delay);
        break;
      case 'startGraphUpdate':
        _algorithmSucceed[graphId] = event.data.algoSucceed;
        //_graphIdToWrapper[graphId].setEventsActivated(false);
        if (!tulip.coreBuild && canvasId) {
          _stopBusyAnimation(canvasId);
          setTimeout(function() {
            _setCurrentCanvas(canvasId);
            _setProgressBarComment(canvasId, "Updating graph visualization ...");
            _startGraphViewUpdate(canvasId);
            currentGraphData = event.data;
            _draw();
          }, delay);
        }
        break;
      case 'endGraphUpdate':
        _fillMetaGraphInfos(_graphIdToWrapper[graphId].cppPointer);
        if (graphId in _algorithmFinishedCallback) {
          _algorithmFinishedCallback[graphId](_algorithmSucceed[graphId], _graphIdToWrapper[graphId]);
        }
        //_graphIdToWrapper[graphId].setEventsActivated(true);

        if (!tulip.coreBuild && canvasId) {
          setTimeout(function() {
            _setCurrentCanvas(canvasId);
            _endGraphViewUpdate(canvasId);
            _setGraphRenderingDataReady(canvasId, true);
            _centerScene(canvasId);
          }, delay);
        }
        break;
      case 'createGraphProperties':
        setTimeout(function() {
          _createGraphProperties(graphId, event.data.properties);
        }, delay);
        break;
      case 'addNodes':
        setTimeout(function() {
          var nodesJson =  event.data.nodesJson;
          _parseNodesJSONData(graphId, nodesJson);
          if (!tulip.coreBuild && canvasId) {
            var nodeId = event.data.lastNodeId;
            var percent = (nodeId / (currentGraphData.numberOfNodes + currentGraphData.numberOfEdges - 1)) * 100;
            _setProgressBarPercent(canvasId, percent);
            _draw();
          }
          _tulipWorker.postMessage({eventType : 'sendNextNodes', graphId : graphId});
        }, delay);
        break;
      case 'addEdges':
        setTimeout(function() {
          var edgesJson = event.data.edgesJson;
          _parseEdgesJSONData(graphId, edgesJson);
          if (!tulip.coreBuild && canvasId) {
            var edgeId = event.data.lastEdgeId;
            var percent = ((currentGraphData.numberOfNodes + edgeId) / (currentGraphData.numberOfNodes + currentGraphData.numberOfEdges - 1)) * 100;
            _setProgressBarPercent(canvasId, percent);
            _draw();
          }
          _tulipWorker.postMessage({eventType : 'sendNextEdges' , graphId : graphId});
        }, delay);
        break;
      case 'addSubGraph':
        setTimeout(function() {
          var subGraphData = event.data.subGraphData;
          _addSubGraph(graphId, subGraphData.parentGraphId, subGraphData.subGraphId, subGraphData.nodesIds, subGraphData.edgesIds, subGraphData.attributes);
          _tulipWorker.postMessage({eventType : 'sendNextSubGraph', graphId : event.data.graphId});
        }, delay);
        break;
      };
    }, false);

    function _sendGraphToWorker(graph, graphFilePath, graphFileData, sendDataBack) {
      if (arguments.length == 1) {
        var file = FS.findObject("/graph.tlpb.gz");
        if (!file) {
          FS.createFile('/', "graph.tlpb.gz", {}, true, true);
        }
        var saved = tulip.saveGraph(graph, "/graph.tlpb.gz");
        console.log(saved);
        var graphData = FS.readFile("/graph.tlpb.gz");
        _sendGraphToWorker(graph, "graph.tlpb.gz", graphData.buffer, false);
      } else {
        var messageData = {
          eventType: 'loadGraph',
          graphId: graph.getCppPointer(),
          graphFile: graphFilePath,
          graphFileData : graphFileData,
          sendDataBack : sendDataBack
        };
        if (graphFileData) {
          _tulipWorker.postMessage(messageData, [messageData.graphFileData]);
        } else {
          _tulipWorker.postMessage(messageData);
        }
      }
    }

  }

  // ==================================================================================================================

  if (!tulip.coreBuild) {

    var _setCanvasGraph = Module.cwrap('setCanvasGraph', null, ['string', 'number']);
    var _getViewRenderingParameters = Module.cwrap('getViewRenderingParameters', 'number', ['string']);

    tulip.getViewForCanvasId = function(canvasId) {
      if (canvasId in _canvasIdToView) {
        return _canvasIdToView[canvasId];
      } else {
        return null;
      }
    };

    tulip.View = function(container, width, height) {
      var newObject = createObject(tulip.View, this);
      if (arguments.length > 0) {
        if (typeof(container) == 'string') {
          newObject.container = document.getElementById(container);
        } else {
          newObject.container = container;
        }

        var currentId = _nextCanvasId++;

        newObject.canvasId = 'tulip-canvas-' + currentId;
        newObject.canvas = document.createElement("canvas");
        newObject.canvas.style.outline = 'none';
        newObject.canvas.id = newObject.canvasId;
        newObject.container.appendChild(newObject.canvas);
        if (typeOf(width) != 'undefined' && typeOf(height) != 'undefined') {
          newObject.sizeRelativeToContainer = false;
        } else {
          newObject.sizeRelativeToContainer = true;
          newObject.canvas.style.width = '100%';
          newObject.canvas.style.height = '100%';
          width = newObject.container.clientWidth;
          height = newObject.container.clientHeight;
        }
        _initCanvas(newObject.canvasId, width, height, newObject.sizeRelativeToContainer);
        newObject.graph = null;
        _canvasIdToView[newObject.canvasId] = newObject;

        newObject.canvas2d = document.createElement("canvas");
        newObject.canvas2dContext = newObject.canvas2d.getContext('2d');
        newObject.canvas2d.id = newObject.canvasId + '-2d';
        newObject.canvas2d.style.display = 'none';
        newObject.canvas2d.width = width;
        newObject.canvas2d.height = height;
        newObject.container.appendChild(newObject.canvas2d);
      }
      newObject.fullScreenActivated = false;
      return newObject;
    };

    tulip.View.prototype.draw = function() {
      if (this.sizeRelativeToContainer) {
        _resizeCanvas(this.canvasId, this.container.clientWidth, this.container.clientHeight, this.sizeRelativeToContainer);
      }
      var view = this;
      Browser.requestAnimationFrame(function() {
        _updateGlScene(view.canvasId);
      });
    };

    function CanvasContextProxy(context, canvasId) {

      var that = this;

      this.beginPath = function() {
        _setCanvas2dModified(canvasId);
        context.beginPath();
      };

      this.arc = function() {
        _setCanvas2dModified(canvasId);
        context.arc(this.arguments);
      }

      this.fillStyle = context.fillStyle;

      this.fill = function() {
        _setCanvas2dModified(canvasId);
        context.fillStyle = this.fillStyle;
        context.fill();
      };

      this.lineWidth = context.lineWidth;
      this.strokeStyle = context.strokeStyle;

      this.stroke = function() {
        _setCanvas2dModified(canvasId);
        context.stroke();
      }

    }

    tulip.View.prototype.getCanvas2dContext = function() {
      if (this.canvas2d.width != this.canvas.width) {
        this.canvas2d.width = this.canvas.width;
        this.canvas2d.height = this.canvas.height;
      }
      return new CanvasContextProxy(this.canvas2dContext, this.canvasId);
    };

    tulip.View.prototype.clearCanvas2d = function() {
      _setCanvas2dModified(this.canvasId);
      this.canvas2dContext.clearRect(0, 0, this.canvas2d.width, this.canvas2d.height);
    }

    tulip.View.prototype.setCurrent = function() {
      _setCurrentCanvas(this.canvasId);
    };

    tulip.View.prototype.activateInteractor = function(interactorName) {
      _activateInteractor(this.canvasId, interactorName);
    };

    tulip.View.prototype.desactivateInteractor = function() {
      _desactivateInteractor(this.canvasId);
    };

    tulip.View.prototype.centerScene = function() {
      _centerScene(this.canvasId);
    };

    tulip.View.prototype.fullScreen = function() {
      _fullScreen(this.canvasId);
      this.canvas2d.width = this.canvas.width;
      this.canvas2d.height = this.canvas.height;
    };

    tulip.View.prototype.resize = function(width, height) {
      _resizeCanvas(this.canvasId, width, height, this.sizeRelativeToContainer);
      this.canvas2d.width = this.canvas.width;
      this.canvas2d.height = this.canvas.height;
    };

    tulip.View.prototype.getWidth = function() {
      return this.canvas.width;
    };

    tulip.View.prototype.setGraph = function(graph) {
      if (!graph.cppPointerValid()) return;
      this.graph = graph;
      _setCanvasGraph(this.canvasId, graph.cppPointer);
      _graphIdToCanvas[graph.getCppPointer()] = this.canvasId;
      _graphIdToWrapper[graph.getCppPointer()] = graph;
    };

    tulip.View.prototype.getGraph = function() {
      return this.graph;
    };

    tulip.View.prototype.loadGraphFromFile = function(graphFilePath, loadGraphInWorker, graphLoadedCallback) {

      var view = this;
      if (loadGraphInWorker) {
        view.graph = tulip.Graph();
        _setCanvasGraph(view.canvasId, view.graph.cppPointer);
        if (graphLoadedCallback) {
          _graphLoadedCallback[view.graph.getCppPointer()] = graphLoadedCallback;
        }
        _graphIdToCanvas[view.graph.getCppPointer()] = view.canvasId;
        _graphIdToWrapper[view.graph.getCppPointer()] = view.graph;
        _setGraphRenderingDataReady(view.canvasId, false);

        _sendGraphToWorker(view.graph, graphFilePath, null, true);
      } else {
        var graphReq = new XMLHttpRequest();
        graphReq.open("GET", graphFilePath, true);
        graphReq.responseType = "arraybuffer";
        graphReq.onload = function (oEvent) {
          var arrayBuffer = graphReq.response;
          var file = FS.findObject(graphFilePath);
          if (!file) {
            var paths = graphFilePath.split('/');
            var filePath = "/";
            for (var i = 0; i < paths.length - 1; ++i) {
              filePath += paths[i];
              filePath += "/";
            }
            FS.createPath('/', filePath, true, true);
            FS.createFile('/', graphFilePath, {}, true, true);
          }
          FS.writeFile(graphFilePath, new Uint8Array(arrayBuffer), {'encoding' : 'binary'});
          view.graph = tulip.loadGraph(graphFilePath, false);
          _setCanvasGraph(view.canvasId, view.graph.cppPointer);
          _graphIdToCanvas[view.graph.getCppPointer()] = view.canvasId;
          _graphIdToWrapper[view.graph.getCppPointer()] = view.graph;
          if (graphLoadedCallback) {
            graphLoadedCallback(view.graph);
          }
        };
        graphReq.send(null);
      }
    };

    tulip.View.prototype.loadGraphFromData = function(graphFilePath, graphFileData, loadGraphInWorker, graphLoadedCallback) {
      var view = this;
      if (loadGraphInWorker) {
        view.graph = tulip.Graph();
        _setCanvasGraph(view.canvasId, view.graph.cppPointer);
        if (graphLoadedCallback) {
          _graphLoadedCallback[view.graph.getCppPointer()] = graphLoadedCallback;
        }
        _setGraphRenderingDataReady(view.canvasId, false);
        _graphIdToCanvas[view.graph.getCppPointer()] = view.canvasId;
        _graphIdToWrapper[view.graph.getCppPointer()] = view.graph;
        _sendGraphToWorker(view.graph, graphFilePath, graphFileData, true);
      } else {
        var file = FS.findObject(graphFilePath);
        if (!file) {
          var paths = graphFilePath.split('/');
          var filePath = "/";
          for (var i = 0; i < paths.length - 1; ++i) {
            filePath += paths[i];
            filePath += "/";
          }
          FS.createPath('/', filePath, true, true);
          FS.createFile('/', graphFilePath, {}, true, true);
        }
        FS.writeFile(graphFilePath, new Uint8Array(graphFileData), {'encoding' : 'binary'});
        view.graph = tulip.loadGraph(graphFilePath, false);
        _setCanvasGraph(view.canvasId, view.graph.cppPointer);
        _graphIdToCanvas[view.graph.getCppPointer()] = view.canvasId;
        _graphIdToWrapper[view.graph.getCppPointer()] = view.graph;
        if (graphLoadedCallback) {
          graphLoadedCallback(view.graph);
        }
      }
    };

    tulip.View.prototype.selectNodesEdges = function(x, y, w, h) {
      if (w == undefined) {
        w = 0;
        h = 0;
      }
      var nbNodes = _selectNodes(this.canvasId, x, y, w, h);
      var nbEdges = _selectEdges(this.canvasId, x, y, w, h);
      var selectedNodes = [];
      var selectedEdges = [];

      if (nbNodes > 0) {
        selectedNodes = getArrayOfTulipType(nbNodes, function(byteOffset) {_getSelectedNodes(byteOffset)}, tulip.Node);
      }

      if (nbEdges > 0) {
        selectedEdges = getArrayOfTulipType(nbEdges, function(byteOffset) {_getSelectedEdges(byteOffset)}, tulip.Edge);
      }

      return {nodes: selectedNodes, edges: selectedEdges};

    };

    tulip.View.prototype.getRenderingParameters = function() {
      return tulip.GlGraphRenderingParameters(_getViewRenderingParameters(this.canvasId));
    }

    // ==================================================================================================

    var _GlGraphRenderingParameters_setDisplayNodes = Module.cwrap('GlGraphRenderingParameters_setDisplayNodes', null, ['number', 'number']);
    var _GlGraphRenderingParameters_displayNodes = Module.cwrap('GlGraphRenderingParameters_displayNodes', 'number', ['number']);
    var _GlGraphRenderingParameters_setBillboardedNodes = Module.cwrap('GlGraphRenderingParameters_setBillboardedNodes', null, ['number', 'number']);
    var _GlGraphRenderingParameters_billboardedNodes = Module.cwrap('GlGraphRenderingParameters_billboardedNodes', 'number', ['number']);
    var _GlGraphRenderingParameters_setDisplayNodesLabels = Module.cwrap('GlGraphRenderingParameters_setDisplayNodesLabels', null, ['number', 'number']);
    var _GlGraphRenderingParameters_displayNodesLabels = Module.cwrap('GlGraphRenderingParameters_displayNodesLabels', 'number', ['number']);
    var _GlGraphRenderingParameters_setLabelsScaled = Module.cwrap('GlGraphRenderingParameters_setLabelsScaled', null, ['number', 'number']);
    var _GlGraphRenderingParameters_labelsScaled = Module.cwrap('GlGraphRenderingParameters_labelsScaled', 'number', ['number']);
    var _GlGraphRenderingParameters_setBillboardedLabels = Module.cwrap('GlGraphRenderingParameters_setBillboardedLabels', null, ['number', 'number']);
    var _GlGraphRenderingParameters_billboardedLabels = Module.cwrap('GlGraphRenderingParameters_billboardedLabels', 'number', ['number']);
    var _GlGraphRenderingParameters_setDisplayEdges = Module.cwrap('GlGraphRenderingParameters_setDisplayEdges', null, ['number', 'number']);
    var _GlGraphRenderingParameters_displayEdges = Module.cwrap('GlGraphRenderingParameters_displayEdges', 'number', ['number']);
    var _GlGraphRenderingParameters_setInterpolateEdgesColors = Module.cwrap('GlGraphRenderingParameters_setInterpolateEdgesColors', null, ['number', 'number']);
    var _GlGraphRenderingParameters_interpolateEdgesColors = Module.cwrap('GlGraphRenderingParameters_interpolateEdgesColors', 'number', ['number']);
    var _GlGraphRenderingParameters_setInterpolateEdgesSizes = Module.cwrap('GlGraphRenderingParameters_setInterpolateEdgesSizes', null, ['number', 'number']);
    var _GlGraphRenderingParameters_interpolateEdgesSizes = Module.cwrap('GlGraphRenderingParameters_interpolateEdgesSizes', 'number', ['number']);
    var _GlGraphRenderingParameters_setDisplayEdgesExtremities = Module.cwrap('GlGraphRenderingParameters_setDisplayEdgesExtremities', null, ['number', 'number']);
    var _GlGraphRenderingParameters_displayEdgesExtremities = Module.cwrap('GlGraphRenderingParameters_displayEdgesExtremities', 'number', ['number']);
    var _GlGraphRenderingParameters_setEdges3D = Module.cwrap('GlGraphRenderingParameters_setEdges3D', null, ['number', 'number']);
    var _GlGraphRenderingParameters_edges3D = Module.cwrap('GlGraphRenderingParameters_edges3D', 'number', ['number']);

    tulip.GlGraphRenderingParameters = function tulip_GlGraphRenderingParameters(cppPointer) {
      var newObject = createObject(tulip.GlGraphRenderingParameters, this);
      tulip.CppObjectWrapper.call(newObject, cppPointer, "GlGraphRenderingParameters");
      return newObject;
    };
    tulip.GlGraphRenderingParameters.inheritsFrom(tulip.CppObjectWrapper);

    tulip.GlGraphRenderingParameters.prototype.setDisplayNodes = function tulip_GlGraphRenderingParameters_prototype_setDisplayNodes(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setDisplayNodes(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.displayNodes = function tulip_GlGraphRenderingParameters_prototype_displayNodes() {
      return _GlGraphRenderingParameters_displayNodes(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setBillboardedNodes = function tulip_GlGraphRenderingParameters_prototype_setBillboardedNodes(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setBillboardedNodes(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.billboardedNodes = function tulip_GlGraphRenderingParameters_prototype_billboardedNodes() {
      return _GlGraphRenderingParameters_billboardedNodes(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setDisplayNodesLabels = function tulip_GlGraphRenderingParameters_prototype_setDisplayNodesLabels(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setDisplayNodesLabels(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.displayNodesLabels = function tulip_GlGraphRenderingParameters_prototype_displayNodesLabels() {
      return _GlGraphRenderingParameters_displayNodesLabels(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setLabelsScaled = function tulip_GlGraphRenderingParameters_prototype_setLabelsScaled(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setLabelsScaled(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.labelsScaled = function tulip_GlGraphRenderingParameters_prototype_labelsScaled() {
      return _GlGraphRenderingParameters_labelsScaled(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setBillboardedLabels = function tulip_GlGraphRenderingParameters_prototype_setBillboardedLabels(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setBillboardedLabels(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.billboardedLabels = function tulip_GlGraphRenderingParameters_prototype_billboardedLabels() {
      return _GlGraphRenderingParameters_billboardedLabels(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setDisplayEdges = function tulip_GlGraphRenderingParameters_prototype_setDisplayEdges(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setDisplayEdges(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.displayEdges = function tulip_GlGraphRenderingParameters_prototype_displayEdges() {
      return _GlGraphRenderingParameters_displayEdges(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setInterpolateEdgesColors = function tulip_GlGraphRenderingParameters_prototype_setInterpolateEdgesColors(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setInterpolateEdgesColors(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.interpolateEdgesColors = function tulip_GlGraphRenderingParameters_prototype_interpolateEdgesColors() {
      return _GlGraphRenderingParameters_interpolateEdgesColors(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setInterpolateEdgesSizes = function tulip_GlGraphRenderingParameters_prototype_setInterpolateEdgesSizes(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setInterpolateEdgesSizes(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.interpolateEdgesSizes = function tulip_GlGraphRenderingParameters_prototype_interpolateEdgesSizes() {
      return _GlGraphRenderingParameters_interpolateEdgesSizes(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setDisplayEdgesExtremities = function tulip_GlGraphRenderingParameters_prototype_setDisplayEdgesExtremities(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setDisplayEdgesExtremities(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.displayEdgesExtremities = function tulip_GlGraphRenderingParameters_prototype_displayEdgesExtremities() {
      return _GlGraphRenderingParameters_displayEdgesExtremities(this.cppPointer) > 0;
    };

    tulip.GlGraphRenderingParameters.prototype.setEdges3D = function tulip_GlGraphRenderingParameters_prototype_setEdges3D(state) {
      checkArgumentsTypes(arguments, ["boolean"], 1);
      _GlGraphRenderingParameters_setEdges3D(this.cppPointer, state);
    };

    tulip.GlGraphRenderingParameters.prototype.edges3D = function tulip_GlGraphRenderingParameters_prototype_edges3D() {
      return _GlGraphRenderingParameters_edges3D(this.cppPointer) > 0;
    };

    tulip.Graph.prototype.getTlpFileBlob = function(gzip) {
      var filename = "/graph.tlp";
      if (gzip) {
        filename += ".gz";
      }
      tulip.saveGraph(this, filename);
      var graphData = FS.readFile(filename);
      return new Blob([graphData.buffer]);
    };

  }

  // ==================================================================================================================


  tulip.isLoaded = function() {
    return tulip.mainCalled && _tulipWorkerInit;
  }

  if (!nodejs) {

    tulip.Graph.prototype.applyAlgorithmInWorker = function(algorithmName, algoParameters, algoFinishedCallback) {
      if (!tulip.algorithmExists(algorithmName)) {
        console.log("Error : no Tulip algorithm named '" + algorithmName + "'");
        return;
      }
      if (algoParameters === undefined) {
        algoParameters = {};
      }
      var graphId = this.getCppPointer();
      if (!tulip.coreBuild && graphId in _graphIdToCanvas) {
        var canvasId = _graphIdToCanvas[graphId];
        _setGraphRenderingDataReady(canvasId, false);
        _startBusyAnimation(canvasId);
        _setProgressBarComment(canvasId, "Applying " + algorithmName + " algorithm ...");
      }
      _sendGraphToWorker(this);
      var messageData = {
        graphId : graphId,
        eventType: 'algorithm',
        algorithmName : algorithmName,
        parameters : JSON.stringify(algoParameters)
      };
      if (algoFinishedCallback) {
        _algorithmFinishedCallback[graphId] = algoFinishedCallback;
      }
      _tulipWorker.postMessage(messageData);
    };

    function applyPropertyAlgorithmInWorker(graph, algorithmName, resultProperty, algoParameters, algoFinishedCallback) {
      if (algoParameters === undefined) {
        algoParameters = {};
      }
      var graphId = graph.getCppPointer();
      _sendGraphToWorker(graph);
      var messageData = {
        graphId : graphId,
        eventType: 'propertyAlgorithm',
        algorithmName : algorithmName,
        resultPropertyName : resultProperty.getName(),
        parameters : JSON.stringify(algoParameters)
      };
      if (algoFinishedCallback) {
        _algorithmFinishedCallback[graphId] = algoFinishedCallback;
      }
      _tulipWorker.postMessage(messageData);
      if (!tulip.coreBuild && graphId in _graphIdToCanvas) {
        var canvasId = _graphIdToCanvas[graphId];
        _setGraphRenderingDataReady(canvasId, false);
        _startBusyAnimation(canvasId);
        _setProgressBarComment(canvasId, "Applying " + algorithmName + " " + resultProperty.getTypename() + " algorithm ...");
      }
    }

    tulip.Graph.prototype.applyDoubleAlgorithmInWorker = function(algorithmName, resultProperty, algoParameters, algoFinishedCallback) {
      if (!tulip.doubleAlgorithmExists(algorithmName)) {
        console.log("Error : no Tulip double algorithm named '" + algorithmName + "'");
        return;
      }
      if (resultProperty instanceof tulip.DoubleProperty) {
        applyPropertyAlgorithmInWorker(this, algorithmName, resultProperty, algoParameters, algoFinishedCallback);
      } else {
        console.log("Error : Second parameter of tulip.Graph.applyDoubleAlgorithm method must be an instance of tulip.DoubleProperty type.");
      }
    };

    tulip.Graph.prototype.applyLayoutAlgorithmInWorker = function(algorithmName, resultProperty, algoParameters, algoFinishedCallback) {
      if (!tulip.layoutAlgorithmExists(algorithmName)) {
        console.log("Error : no Tulip layout algorithm named '" + algorithmName + "'");
        return;
      }
      if (resultProperty instanceof tulip.LayoutProperty) {
        applyPropertyAlgorithmInWorker(this, algorithmName, resultProperty, algoParameters, algoFinishedCallback);
      } else {
        console.log("Error : Second parameter of tulip.Graph.applyLayoutAlgorithm method must be an instance of tulip.LayoutProperty type.");
      }
    };

    tulip.Graph.prototype.applySizeAlgorithmInWorker = function(algorithmName, resultProperty, algoParameters, algoFinishedCallback) {
      if (!tulip.sizeAlgorithmExists(algorithmName)) {
        console.log("Error : no Tulip size algorithm named '" + algorithmName + "'");
        return;
      }
      if (resultProperty instanceof tulip.SizeProperty) {
        applyPropertyAlgorithmInWorker(this, algorithmName, resultProperty, algoParameters, algoFinishedCallback);
      } else {
        console.log("Error : Second parameter of tulip.Graph.applySizeAlgorithm method must be an instance of tulip.SizeProperty type.");
      }
    };

    tulip.Graph.prototype.executeScriptInWorker = function(graphFunction, scriptExecutedCallback) {
      var graphId = this.getCppPointer();
      _sendGraphToWorker(this);
      if (scriptExecutedCallback) {
        _algorithmFinishedCallback[graphId] = scriptExecutedCallback;
      }
      if (!tulip.coreBuild && graphId in _graphIdToCanvas) {
        var canvasId = _graphIdToCanvas[graphId];
        _setGraphRenderingDataReady(canvasId, false);
        _startBusyAnimation(canvasId);
        _setProgressBarComment(canvasId, "Executing script on graph ...");
      }
      _tulipWorker.postMessage({
                                 graphId: this.getCppPointer(),
                                 eventType: 'executeGraphScript',
                                 scriptCode: graphFunction.toString()
                               });
    };

  }

  // ==================================================================================================================
}
