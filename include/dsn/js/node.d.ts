export * from "./index";

export interface GeneratedTypeModule {
    file: string;
    exports: string[];
}

export function loadGeneratedTypes(
    filename: string,
    exportNames: string[],
    dependencies?: Record<string, unknown>
): Record<string, unknown>;

export function loadGeneratedTypeModules(
    directory: string,
    modules: GeneratedTypeModule[]
): Record<string, unknown>;

declare const nodeRuntime: typeof import("./index") & {
    loadGeneratedTypes: typeof loadGeneratedTypes;
    loadGeneratedTypeModules: typeof loadGeneratedTypeModules;
};

export default nodeRuntime;
