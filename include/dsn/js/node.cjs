"use strict";

const fs = require("fs");
const path = require("path");
const vm = require("vm");
const runtime = require("./index.cjs");

function loadGeneratedTypes(filename, exportNames, dependencies) {
    if (!Array.isArray(exportNames)) {
        throw new TypeError("Generated type export names must be an array");
    }
    const source = fs.readFileSync(filename, "utf8");
    const sandbox = Object.assign(Object.create(null), runtime.types, dependencies || {});
    sandbox.Thrift = runtime.Thrift;
    vm.createContext(sandbox);
    new vm.Script(source, { filename }).runInContext(sandbox);

    const result = Object.create(null);
    for (const name of exportNames) {
        if (typeof name !== "string" || !Object.prototype.hasOwnProperty.call(sandbox, name)) {
            throw new Error(`Generated Thrift file ${filename} did not define ${name}`);
        }
        result[name] = sandbox[name];
    }
    return result;
}

function loadGeneratedTypeModules(directory, modules) {
    if (!Array.isArray(modules)) {
        throw new TypeError("Generated type modules must be an array");
    }
    const result = Object.assign(Object.create(null), runtime.types);
    for (const module of modules) {
        if (!module || typeof module.file !== "string" || !Array.isArray(module.exports)) {
            throw new TypeError("Each generated type module needs file and exports properties");
        }
        Object.assign(
            result,
            loadGeneratedTypes(path.resolve(directory, module.file), module.exports, result));
    }
    return result;
}

module.exports = Object.assign({}, runtime, {
    loadGeneratedTypes,
    loadGeneratedTypeModules
});
