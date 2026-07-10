export type DsnHeaderValue = string | number | boolean | null | undefined;
export type DsnHeaders =
    | Record<string, DsnHeaderValue>
    | Array<[string, DsnHeaderValue]>
    | { forEach(callback: (value: unknown, name: unknown) => void): void };

export interface DsnAbortSignal {
    readonly aborted: boolean;
    addEventListener(name: "abort", callback: () => void, options?: { once?: boolean }): void;
    removeEventListener(name: "abort", callback: () => void): void;
}

export interface DsnFetchResponse {
    readonly ok: boolean;
    readonly status: number;
    readonly headers?: { get(name: string): string | null };
    text(): Promise<string>;
}

export interface DsnFetchOptions {
    method: string;
    headers: Record<string, string>;
    body: string;
    credentials?: "omit" | "same-origin" | "include";
    signal?: DsnAbortSignal;
}

export interface DsnCallOptions {
    hash?: number | string;
    timeoutMs?: number;
    signal?: DsnAbortSignal;
    headers?: DsnHeaders;
    credentials?: "omit" | "same-origin" | "include";
    fetch?: (
        url: string,
        options: DsnFetchOptions
    ) => Promise<DsnFetchResponse>;
}

export interface DsnCallbackOptions<TArgs, TResult> extends DsnCallOptions {
    args: TArgs;
    async?: boolean;
    on_success?: (result: TResult) => void;
    on_fail?: (response: unknown, status: string, error: DsnRpcError) => void;
}

export class DsnRpcError extends Error {
    readonly code: string;
    readonly status: number;
    readonly response: unknown;
    readonly responseText: unknown;
    readonly cause?: unknown;
}

export interface DsnGeneratedMethod {
    name: string;
    rpcCode: string;
    requestType: string;
    responseType: string;
    payloadFormat: string;
    oneWay?: boolean;
}

export interface DsnGeneratedClient {
    readonly url: string;
    readonly options: DsnCallOptions;
    readonly types: Record<string, unknown>;
}

export interface DsnGeneratedClientConstructor {
    new (
        endpoint: string,
        options?: DsnCallOptions & { types?: Record<string, unknown> }
    ): DsnGeneratedClient;
    readonly serviceName: string;
}

export const Thrift: Record<string, unknown>;
export const DSN: Record<string, unknown>;
export const types: {
    error_code: typeof error_code;
    task_code: typeof task_code;
    gpid: typeof gpid;
    blob: typeof blob;
    rpc_address: typeof rpc_address;
};

export function dsn_call(
    url: string,
    rpcCode: string,
    hash: number | string | undefined,
    method: string,
    data: string,
    payloadFormat: string,
    async: boolean,
    onSuccess?: (response: string) => unknown,
    onFail?: (response: unknown, status: string, error: DsnRpcError) => unknown,
    options?: DsnCallOptions
): unknown;

export function marshall_thrift_json(
    value: unknown,
    type: string,
    registry?: Record<string, unknown>
): string;
export function unmarshall_thrift_json(
    buffer: string,
    value: unknown,
    type: string,
    registry?: Record<string, unknown>
): unknown;
export function createClientClass(
    serviceName: string,
    methods: DsnGeneratedMethod[],
    generatedTypes?: Record<string, unknown>
): DsnGeneratedClientConstructor;
export function normalizeUrl(url: string): string;

export class error_code {
    constructor(args?: string | { code?: string });
    code: string;
    toString(): string;
}

export class task_code {
    constructor(args?: string | { code?: string });
    code: string;
    toString(): string;
}

export class gpid {
    constructor(args?: number | string | bigint | {
        id?: number | string | bigint;
        app_id?: number;
        partition_index?: number;
    });
    id: number | string;
    static from(appId: number, partitionIndex: number): gpid;
    app_id(): number;
    partition_index(): number;
    get_app_id(): number;
    get_partition_index(): number;
    set_app_id(value: number): void;
    set_partition_index(value: number): void;
    toString(): string;
}

export class blob {
    constructor(args?: string | Uint8Array | ArrayBuffer | { data?: unknown });
    data: unknown;
    toString(): string;
}

export class rpc_address {
    constructor(args?: string | { host?: string; port?: number });
    host: string;
    port: number;
    is_invalid(): boolean;
    toString(): string;
}

declare const runtime: {
    Thrift: typeof Thrift;
    DSN: typeof DSN;
    DsnRpcError: typeof DsnRpcError;
    dsn_call: typeof dsn_call;
    marshall_thrift_json: typeof marshall_thrift_json;
    unmarshall_thrift_json: typeof unmarshall_thrift_json;
    createClientClass: typeof createClientClass;
    normalizeUrl: typeof normalizeUrl;
    types: typeof types;
    error_code: typeof error_code;
    task_code: typeof task_code;
    gpid: typeof gpid;
    blob: typeof blob;
    rpc_address: typeof rpc_address;
};

export default runtime;
