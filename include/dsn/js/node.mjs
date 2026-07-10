import nodeRuntime from "./node.cjs";

export * from "./index.mjs";
export const loadGeneratedTypes = nodeRuntime.loadGeneratedTypes;
export const loadGeneratedTypeModules = nodeRuntime.loadGeneratedTypeModules;

export default nodeRuntime;
