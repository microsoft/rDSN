var dsn_global = typeof globalThis !== 'undefined' ? globalThis :
    (typeof window !== 'undefined' ? window :
    (typeof self !== 'undefined' ? self :
    (typeof global !== 'undefined' ? global : null)));
var dsn_commonjs =
    typeof module !== 'undefined' && module.exports &&
    typeof require === 'function';
var Thrift = dsn_commonjs ? require('./thrift.js') :
    dsn_global && dsn_global.Thrift;
if (!Thrift) {
    throw new Error('dsn_transport.js requires thrift.js');
}

Thrift.DSNTransport = function(buffer) {
    this.wpos = 0;
    this.rpos = 0;
    this.useCORS = null;
    if (buffer == undefined) {
        this.send_buf = "";
        this.recv_buf = "";
    } else {
        this.send_buf = this.recv_buf = buffer;
        this.wpos = buffer.length;
    }
};

Thrift.DSNTransport.prototype = {
    /**
     * Returns true if the transport is open, XHR always returns true.
     * @readonly
     * @returns {boolean} Always True.
     */    
    isOpen: function() {
        return true;
    },

    /**
     * Opens the transport connection, with XHR this is a nop.
     */    
    open: function() {},

    /**
     * Closes the transport connection, with XHR this is a nop.
     */    
    close: function() {},

    /**
     * Returns the specified number of characters from the response
     * buffer.
     * @param {number} len - The number of characters to return.
     * @returns {string} Characters sent by the server.
     */
    read: function(len) {
        var avail = this.wpos - this.rpos;

        if (avail === 0) {
            return '';
        }

        var give = len;

        if (avail < len) {
            give = avail;
        }

        var ret = this.recv_buf.substr(this.rpos, give);
        this.rpos += give;

        //clear buf when complete?
        return ret;
    },

    /**
     * Returns the entire response buffer.
     * @returns {string} Characters sent by the server.
     */
    readAll: function() {
        return this.recv_buf;
    },

    /**
     * Sets the send buffer to buf.
     * @param {string} buf - The buffer to send.
     */    
    write: function(buf) {
        this.send_buf = buf;
    },

    /**
     * Returns the send buffer.
     * @readonly
     * @returns {string} The send buffer.
     */ 
    getSendBuffer: function() {
        return this.send_buf;
    }

};

var DSN = {
    thrift_type : {
        "bool" : Thrift.Type.BOOL,
        "byte" : Thrift.Type.BYTE,
        "i16" : Thrift.Type.I16,
        "i32" : Thrift.Type.I32,
        "i64" : Thrift.Type.I64,
        "double" : Thrift.Type.DOUBLE,
        "string" : Thrift.Type.STRING,
        "binary" : Thrift.Type.STRING,
        "struct" : Thrift.Type.STRUCT,
        "map" : Thrift.Type.MAP,
        "list" : Thrift.Type.LIST,
        "set" : Thrift.Type.SET,
        "vector" : Thrift.Type.LIST
    }
};

DSN.base_type = {
    "bool": { kind: "bool", thrift_type: Thrift.Type.BOOL },
    "BOOL": { kind: "bool", thrift_type: Thrift.Type.BOOL },
    "Bool": { kind: "bool", thrift_type: Thrift.Type.BOOL },
    "byte": { kind: "byte", thrift_type: Thrift.Type.BYTE },
    "BYTE": { kind: "byte", thrift_type: Thrift.Type.BYTE },
    "Byte": { kind: "byte", thrift_type: Thrift.Type.BYTE },
    "i8": { kind: "byte", thrift_type: Thrift.Type.BYTE },
    "int8": { kind: "byte", thrift_type: Thrift.Type.BYTE },
    "int8_t": { kind: "byte", thrift_type: Thrift.Type.BYTE },
    "ui8": { kind: "byte", thrift_type: Thrift.Type.BYTE, unsigned: true },
    "uint8": { kind: "byte", thrift_type: Thrift.Type.BYTE, unsigned: true },
    "uint8_t": { kind: "byte", thrift_type: Thrift.Type.BYTE, unsigned: true },
    "i16": { kind: "i16", thrift_type: Thrift.Type.I16 },
    "int16": { kind: "i16", thrift_type: Thrift.Type.I16 },
    "int16_t": { kind: "i16", thrift_type: Thrift.Type.I16 },
    "ui16": { kind: "i16", thrift_type: Thrift.Type.I16, unsigned: true },
    "uint16": { kind: "i16", thrift_type: Thrift.Type.I16, unsigned: true },
    "uint16_t": { kind: "i16", thrift_type: Thrift.Type.I16, unsigned: true },
    "i32": { kind: "i32", thrift_type: Thrift.Type.I32 },
    "int32": { kind: "i32", thrift_type: Thrift.Type.I32 },
    "int32_t": { kind: "i32", thrift_type: Thrift.Type.I32 },
    "sint32": { kind: "i32", thrift_type: Thrift.Type.I32 },
    "fixed32": { kind: "i32", thrift_type: Thrift.Type.I32 },
    "sfixed32": { kind: "i32", thrift_type: Thrift.Type.I32 },
    "ui32": { kind: "i32", thrift_type: Thrift.Type.I32, unsigned: true },
    "uint32": { kind: "i32", thrift_type: Thrift.Type.I32, unsigned: true },
    "uint32_t": { kind: "i32", thrift_type: Thrift.Type.I32, unsigned: true },
    "i64": { kind: "i64", thrift_type: Thrift.Type.I64 },
    "int64": { kind: "i64", thrift_type: Thrift.Type.I64 },
    "int64_t": { kind: "i64", thrift_type: Thrift.Type.I64 },
    "sint64": { kind: "i64", thrift_type: Thrift.Type.I64 },
    "fixed64": { kind: "i64", thrift_type: Thrift.Type.I64 },
    "sfixed64": { kind: "i64", thrift_type: Thrift.Type.I64 },
    "ui64": { kind: "i64", thrift_type: Thrift.Type.I64, unsigned: true },
    "uint64": { kind: "i64", thrift_type: Thrift.Type.I64, unsigned: true },
    "uint64_t": { kind: "i64", thrift_type: Thrift.Type.I64, unsigned: true },
    "double": { kind: "double", thrift_type: Thrift.Type.DOUBLE },
    "float": { kind: "double", thrift_type: Thrift.Type.DOUBLE },
    "string": { kind: "string", thrift_type: Thrift.Type.STRING },
    "binary": { kind: "binary", thrift_type: Thrift.Type.STRING },
    "struct": { kind: "struct", thrift_type: Thrift.Type.STRUCT }
};

DSN.type_registry = Object.create(null);

DSN.register_types = function(types) {
    if (!types || typeof types !== 'object') {
        throw new TypeError('Type registry must be an object');
    }
    for (var name in types) {
        if (Object.prototype.hasOwnProperty.call(types, name)) {
            DSN.type_registry[name] = types[name];
        }
    }
};

DSN.DsnRpcError = function(code, message, response, responseText, cause) {
    this.name = "DsnRpcError";
    this.code = code || "TRANSPORT_ERROR";
    this.message = message || this.code;
    this.response = response || null;
    this.responseText = responseText === undefined ? null : responseText;
    this.status = response && typeof response.status === "number" ? response.status : 0;
    if (cause !== undefined && cause !== null) {
        this.cause = cause;
    }
    if (Error.captureStackTrace) {
        Error.captureStackTrace(this, DSN.DsnRpcError);
    }
};

DSN.DsnRpcError.prototype = Object.create(Error.prototype);
DSN.DsnRpcError.prototype.constructor = DSN.DsnRpcError;
DSN.RpcError = DSN.DsnRpcError;

DSN.get_response_header = function(response, name) {
    if (!response) {
        return null;
    }
    if (response.headers && typeof response.headers.get === "function") {
        return response.headers.get(name);
    }
    if (typeof response.getResponseHeader === "function") {
        return response.getResponseHeader(name);
    }
    return null;
};

DSN.get_response_error = function(response, responseText) {
    var status = response && typeof response.status === "number" ? response.status : 0;
    if (response && response.readyState === 4 && status === 0) {
        return new DSN.DsnRpcError(
            "TRANSPORT_ERROR",
            "rDSN RPC transport failed",
            response,
            responseText);
    }
    if ((response && response.ok === false) || (status !== 0 && (status < 200 || status >= 300))) {
        return new DSN.DsnRpcError(
            status === 0 ? "HTTP_ERROR" : "HTTP_" + status,
            status === 0 ? "rDSN RPC HTTP request failed" :
                "rDSN RPC failed with HTTP status " + status,
            response,
            responseText);
    }

    var serverError = DSN.get_response_header(response, "server_error");
    if (serverError !== null && serverError !== undefined) {
        serverError = String(serverError).replace(/^\s+|\s+$/g, "");
        if (serverError !== "" && serverError !== "ERR_OK") {
            return new DSN.DsnRpcError(
                serverError,
                "rDSN RPC failed with " + serverError,
                response,
                responseText);
        }
    }
    return null;
};

DSN.normalize_rpc_error = function(response, textStatus, errorThrown, requestState) {
    if (errorThrown instanceof DSN.DsnRpcError) {
        return errorThrown;
    }

    if ((requestState && requestState.timedOut) || textStatus === 'timeout') {
        return new DSN.DsnRpcError(
            "CLIENT_TIMEOUT",
            "rDSN RPC exceeded the client timeout",
            response,
            response && response.responseText,
            errorThrown);
    }
    if ((requestState && requestState.cancelled) || textStatus === 'abort') {
        return new DSN.DsnRpcError(
            "CANCELLED",
            "rDSN RPC was cancelled",
            response,
            response && response.responseText,
            errorThrown);
    }

    var status = response && typeof response.status === "number" ? response.status : 0;
    var code = status === 0 ? "TRANSPORT_ERROR" : "HTTP_" + status;
    var message;
    if (errorThrown && errorThrown.message) {
        message = errorThrown.message;
    } else if (errorThrown) {
        message = String(errorThrown);
    } else if (textStatus) {
        message = "rDSN RPC failed: " + textStatus;
    } else {
        message = "rDSN RPC transport failed";
    }
    return new DSN.DsnRpcError(
        code,
        message,
        response,
        response && response.responseText,
        errorThrown);
};

DSN.resolve_type = function(name, registry) {
    var normalized = name.replace(/::/g, '.').replace(/^\.+/, '');
    var parts = normalized.split('.');
    var shortName = parts[parts.length - 1];
    if (registry) {
        if (registry[normalized] !== undefined) {
            return registry[normalized];
        }
        if (registry[shortName] !== undefined) {
            return registry[shortName];
        }
    }
    if (DSN.type_registry[normalized] !== undefined) {
        return DSN.type_registry[normalized];
    }
    if (DSN.type_registry[shortName] !== undefined) {
        return DSN.type_registry[shortName];
    }

    var root = dsn_global;
    if (!root) {
        return undefined;
    }
    var value = root;
    for (var i = 0; i < parts.length && value !== undefined; ++i) {
        value = value[parts[i]];
    }
    if (value !== undefined) {
        return value;
    }
    return root[parts[parts.length - 1]];
};

DSN.parse_type = function(type, registry) {
    if (typeof type !== 'string') {
        throw new TypeError('Thrift type must be a string');
    }
    var name = type.replace(/\s+/g, '');
    if (DSN.base_type[name] !== undefined) {
        return DSN.base_type[name];
    }

    var open = name.indexOf('<');
    if (open > 0 && name.charAt(name.length - 1) === '>') {
        var container = name.substring(0, open);
        var inner = name.substring(open + 1, name.length - 1);
        if (container === 'vector' || container === 'list' || container === 'set') {
            return {
                kind: container,
                thrift_type: container === 'set' ? Thrift.Type.SET : Thrift.Type.LIST,
                element: DSN.parse_type(inner, registry)
            };
        }
        if (container === 'map') {
            var depth = 0;
            var comma = -1;
            for (var i = 0; i < inner.length; ++i) {
                var ch = inner.charAt(i);
                if (ch === '<') {
                    ++depth;
                } else if (ch === '>') {
                    --depth;
                } else if (ch === ',' && depth === 0) {
                    comma = i;
                    break;
                }
            }
            if (comma < 0) {
                throw new Error('Invalid map type: ' + type);
            }
            return {
                kind: 'map',
                thrift_type: Thrift.Type.MAP,
                key: DSN.parse_type(inner.substring(0, comma), registry),
                value: DSN.parse_type(inner.substring(comma + 1), registry)
            };
        }
    }

    var resolved = DSN.resolve_type(name, registry);
    if (resolved !== undefined && typeof resolved !== 'function') {
        var enumValue = false;
        for (var key in resolved) {
            if (resolved.hasOwnProperty(key)) {
                if (typeof resolved[key] !== 'number') {
                    enumValue = false;
                    break;
                }
                enumValue = true;
            }
        }
        if (enumValue) {
            return { kind: 'i32', thrift_type: Thrift.Type.I32, enum_type: name };
        }
    }
    return {
        kind: 'struct',
        thrift_type: Thrift.Type.STRUCT,
        name: name,
        constructor: typeof resolved === 'function' ? resolved : undefined
    };
};

DSN.normalize_integer = function(value, bits, unsigned, reading) {
    var number = Number(value);
    var unsignedLimit = Math.pow(2, bits);
    var signedLimit = Math.pow(2, bits - 1);
    if (!isFinite(number) || Math.floor(number) !== number) {
        throw new TypeError('Expected an integer value');
    }
    if (reading && unsigned) {
        if (number < -signedLimit || number > signedLimit - 1) {
            throw new RangeError('Integer value is out of range for ' + bits + ' bits');
        }
        return number < 0 ? number + unsignedLimit : number;
    }
    var min = unsigned ? 0 : -signedLimit;
    var max = unsigned ? unsignedLimit - 1 : signedLimit - 1;
    if (number < min || number > max) {
        throw new RangeError('Integer value is out of range for ' + bits + ' bits');
    }
    return unsigned && number >= signedLimit ? number - unsignedLimit : number;
};

DSN.normalize_bool = function(value) {
    if (value === true || value === 1 || value === '1' || value === 'true') {
        return true;
    }
    if (value === false || value === 0 || value === '0' || value === 'false') {
        return false;
    }
    throw new TypeError('Expected a boolean value');
};

DSN.write_value = function(value, descriptor, protocol) {
    switch (descriptor.kind) {
        case 'bool':
            protocol.writeBool(DSN.normalize_bool(value));
            return;
        case 'byte':
            protocol.writeByte(DSN.normalize_integer(value, 8, descriptor.unsigned, false));
            return;
        case 'i16':
            protocol.writeI16(DSN.normalize_integer(value, 16, descriptor.unsigned, false));
            return;
        case 'i32':
            protocol.writeI32(DSN.normalize_integer(value, 32, descriptor.unsigned, false));
            return;
        case 'i64':
            protocol.writeI64(descriptor.unsigned ?
                Thrift.Int64.toSignedDecimalString(value, true) : value);
            return;
        case 'double':
            protocol.writeDouble(value);
            return;
        case 'string':
            protocol.writeString(value);
            return;
        case 'binary':
            protocol.writeBinary(value);
            return;
        case 'struct':
            if (value === null || value === undefined || typeof value.write !== 'function') {
                throw new TypeError('Expected a Thrift struct with a write() method');
            }
            value.write(protocol);
            return;
        case 'vector':
        case 'list':
        case 'set':
            var items = value;
            if (!Array.isArray(items)) {
                if (items !== null && items !== undefined &&
                    typeof items.forEach === 'function') {
                    var converted = [];
                    items.forEach(function(item) { converted.push(item); });
                    items = converted;
                } else {
                    throw new TypeError('Expected an array or set');
                }
            }
            if (descriptor.kind === 'set') {
                Thrift.checkSetUniqueness(items);
                protocol.writeSetBegin(descriptor.element.thrift_type, items.length);
            } else {
                protocol.writeListBegin(descriptor.element.thrift_type, items.length);
            }
            for (var i = 0; i < items.length; ++i) {
                DSN.write_value(items[i], descriptor.element, protocol);
            }
            if (descriptor.kind === 'set') {
                protocol.writeSetEnd();
            } else {
                protocol.writeListEnd();
            }
            return;
        case 'map':
            var entries = [];
            if (typeof Map !== 'undefined' && value instanceof Map) {
                value.forEach(function(mapValue, mapKey) {
                    entries.push([mapKey, mapValue]);
                });
            } else if (value !== null && typeof value === 'object') {
                for (var key in value) {
                    if (Object.prototype.hasOwnProperty.call(value, key)) {
                        entries.push([key, value[key]]);
                    }
                }
            } else {
                throw new TypeError('Expected a map or object');
            }
            protocol.writeMapBegin(
                descriptor.key.thrift_type, descriptor.value.thrift_type, entries.length);
            for (var entry = 0; entry < entries.length; ++entry) {
                DSN.write_value(entries[entry][0], descriptor.key, protocol);
                DSN.write_value(entries[entry][1], descriptor.value, protocol);
            }
            protocol.writeMapEnd();
            return;
    }
    throw new Error('Unsupported Thrift type: ' + descriptor.kind);
};

DSN.read_value = function(protocol, descriptor, target) {
    var result;
    switch (descriptor.kind) {
        case 'bool':
            return protocol.readBool().value;
        case 'byte':
            result = protocol.readByte().value;
            return DSN.normalize_integer(result, 8, descriptor.unsigned, true);
        case 'i16':
            result = protocol.readI16().value;
            return DSN.normalize_integer(result, 16, descriptor.unsigned, true);
        case 'i32':
            result = protocol.readI32().value;
            return DSN.normalize_integer(result, 32, descriptor.unsigned, true);
        case 'i64':
            result = protocol.readI64().value;
            return descriptor.unsigned ? Thrift.Int64.normalizeUnsigned(result) : result;
        case 'double':
            return protocol.readDouble().value;
        case 'string':
            return protocol.readString().value;
        case 'binary':
            return protocol.readBinary().value;
        case 'struct':
            if (target === null || target === undefined) {
                var constructor =
                    descriptor.constructor || DSN.resolve_type(descriptor.name);
                if (typeof constructor !== 'function') {
                    throw new Error('Unknown Thrift struct type: ' + descriptor.name);
                }
                target = new constructor();
            }
            if (typeof target.read !== 'function') {
                throw new TypeError('Expected a Thrift struct with a read() method');
            }
            target.read(protocol);
            return target;
        case 'vector':
        case 'list':
        case 'set':
            var list = descriptor.kind === 'set' ?
                protocol.readSetBegin() : protocol.readListBegin();
            result = [];
            for (var i = 0; i < list.size; ++i) {
                result.push(DSN.read_value(protocol, descriptor.element, null));
            }
            if (descriptor.kind === 'set') {
                protocol.readSetEnd();
                Thrift.checkSetUniqueness(result);
            } else {
                protocol.readListEnd();
            }
            return result;
        case 'map':
            var map = protocol.readMapBegin();
            result = {};
            for (var entry = 0; entry < map.size; ++entry) {
                while (entry > 0 &&
                       protocol.rstack.length >
                           protocol.rpos[protocol.rpos.length - 1] + 1) {
                    protocol.rstack.pop();
                }
                var key = DSN.read_value(protocol, descriptor.key, null);
                var mapValue = DSN.read_value(protocol, descriptor.value, null);
                if (key === '__proto__') {
                    Object.defineProperty(result, key, {
                        value: mapValue,
                        enumerable: true,
                        configurable: true,
                        writable: true
                    });
                } else {
                    result[key] = mapValue;
                }
            }
            protocol.readMapEnd();
            return result;
    }
    throw new Error('Unsupported Thrift type: ' + descriptor.kind);
};

DSN.set_header = function(headers, name, value, preserveRemoval) {
    if (typeof name !== 'string' ||
        !/^[!#$%&'*+\-.^_`|~0-9A-Za-z]+$/.test(name)) {
        throw new TypeError('Invalid HTTP header name');
    }
    var existing;
    for (var key in headers) {
        if (Object.prototype.hasOwnProperty.call(headers, key) &&
            key.toLowerCase() === name.toLowerCase()) {
            existing = key;
            break;
        }
    }
    if (value === null || value === undefined) {
        if (preserveRemoval) {
            headers[existing === undefined ? name : existing] = null;
        } else if (existing !== undefined) {
            delete headers[existing];
        }
        return;
    }
    var text = String(value);
    if (/[\r\n]/.test(text)) {
        throw new TypeError('HTTP header values must not contain line breaks');
    }
    headers[existing === undefined ? name : existing] = text;
};

DSN.copy_headers = function(target, source, preserveRemoval) {
    if (!source) {
        return target;
    }
    if (typeof source.forEach === 'function' && !Array.isArray(source)) {
        source.forEach(function(value, name) {
            DSN.set_header(target, String(name), value, preserveRemoval);
        });
        return target;
    }
    if (Array.isArray(source)) {
        for (var pair = 0; pair < source.length; ++pair) {
            if (!Array.isArray(source[pair]) || source[pair].length !== 2) {
                throw new TypeError('HTTP header entries must contain a name and value');
            }
            DSN.set_header(
                target, String(source[pair][0]), source[pair][1], preserveRemoval);
        }
        return target;
    }
    if (typeof source !== 'object') {
        throw new TypeError('HTTP headers must be an object, array, or Headers instance');
    }
    for (var name in source) {
        if (Object.prototype.hasOwnProperty.call(source, name)) {
            DSN.set_header(target, name, source[name], preserveRemoval);
        }
    }
    return target;
};

DSN.find_header = function(headers, name) {
    if (headers) {
        for (var key in headers) {
            if (Object.prototype.hasOwnProperty.call(headers, key) &&
                key.toLowerCase() === name.toLowerCase()) {
                return { found: true, value: headers[key] };
            }
        }
    }
    return { found: false, value: undefined };
};

DSN.normalize_timeout = function(timeoutMs) {
    if (timeoutMs === undefined || timeoutMs === null) {
        return 0;
    }
    var timeout = Number(timeoutMs);
    if (!isFinite(timeout) || Math.floor(timeout) !== timeout || timeout < 0) {
        throw new RangeError('timeoutMs must be a non-negative integer');
    }
    return timeout;
};

DSN.merge_options = function(base, override) {
    var result = {};
    var copy = function(source) {
        if (!source) {
            return;
        }
        if (typeof source !== 'object') {
            throw new TypeError('rDSN client options must be an object');
        }
        var keys = ['timeoutMs', 'signal', 'credentials', 'fetch'];
        for (var i = 0; i < keys.length; ++i) {
            if (source[keys[i]] !== undefined) {
                result[keys[i]] = source[keys[i]];
            }
        }
    };
    copy(base);
    copy(override);
    result.timeoutMs = DSN.normalize_timeout(result.timeoutMs);
    if (result.credentials !== undefined &&
        result.credentials !== 'omit' &&
        result.credentials !== 'same-origin' &&
        result.credentials !== 'include') {
        throw new TypeError(
            'credentials must be omit, same-origin, or include');
    }
    if (result.fetch !== undefined && typeof result.fetch !== 'function') {
        throw new TypeError('fetch must be a function');
    }
    result.headers = {};
    DSN.copy_headers(result.headers, base && base.headers, true);
    DSN.copy_headers(result.headers, override && override.headers, true);
    return result;
};

DSN.merge_types = function(base, override) {
    var result = Object.create(null);
    var copy = function(source) {
        if (!source) {
            return;
        }
        if (typeof source !== 'object') {
            throw new TypeError('Generated type registry must be an object');
        }
        for (var name in source) {
            if (Object.prototype.hasOwnProperty.call(source, name)) {
                result[name] = source[name];
            }
        }
    };
    copy(base);
    copy(override);
    return result;
};

DSN.normalize_url = function(value) {
    if (typeof value !== 'string' || value.replace(/^\s+|\s+$/g, '') === '') {
        throw new DSN.DsnRpcError(
            'INVALID_URL', 'rDSN RPC URL must be a non-empty string');
    }
    var text = value.replace(/^\s+|\s+$/g, '');
    if (!/^https?:\/\//i.test(text)) {
        throw new DSN.DsnRpcError(
            'INVALID_URL', 'rDSN RPC URL must use HTTP or HTTPS');
    }
    if (typeof URL !== 'undefined') {
        var parsed;
        try {
            parsed = new URL(text);
        } catch (error) {
            throw new DSN.DsnRpcError(
                'INVALID_URL', 'Invalid rDSN RPC URL: ' + text, null, null, error);
        }
        if (parsed.protocol !== 'http:' && parsed.protocol !== 'https:') {
            throw new DSN.DsnRpcError(
                'INVALID_URL', 'rDSN RPC URL must use HTTP or HTTPS');
        }
        if (parsed.username || parsed.password) {
            throw new DSN.DsnRpcError(
                'INVALID_URL', 'rDSN RPC URL must not contain credentials');
        }
        if (parsed.search || text.indexOf('?') >= 0) {
            throw new DSN.DsnRpcError(
                'INVALID_URL', 'rDSN RPC URL must not contain a query string');
        }
        if (parsed.hash || text.indexOf('#') >= 0) {
            throw new DSN.DsnRpcError(
                'INVALID_URL', 'rDSN RPC URL must not contain a fragment');
        }
        return parsed.href.replace(/\/+$/, '');
    }
    if (!/^https?:\/\/[^/?#]+(?:\/[^?#]*)?$/i.test(text)) {
        throw new DSN.DsnRpcError(
            'INVALID_URL', 'Invalid rDSN RPC URL: ' + text);
    }
    if (/^https?:\/\/[^/]*@/i.test(text)) {
        throw new DSN.DsnRpcError(
            'INVALID_URL', 'rDSN RPC URL must not contain credentials');
    }
    return text.replace(/\/+$/, '');
};

DSN.build_rpc_url = function(url, payloadFormat, hash, rpcCode) {
    if (typeof payloadFormat !== 'string' || payloadFormat.length === 0) {
        throw new TypeError('rDSN payload format must be a non-empty string');
    }
    if (typeof rpcCode !== 'string' || rpcCode.length === 0) {
        throw new TypeError('rDSN RPC code must be a non-empty string');
    }
    return DSN.normalize_url(url) + '/' + encodeURIComponent(payloadFormat) + '/' +
        encodeURIComponent(hash === undefined || hash === null ? 0 : hash) + '/' +
        encodeURIComponent(rpcCode);
};

DSN.build_headers = function(options) {
    var headers = {};
    DSN.set_header(headers, 'Accept', 'text/plain, application/json, */*');
    DSN.set_header(
        headers, 'Content-Type', 'application/vnd.apache.thrift.json; charset=utf-8');
    DSN.copy_headers(headers, options && options.headers);
    return headers;
};

DSN.create_request_state = function(options) {
    var state = {
        timedOut: false,
        cancelled: false,
        timer: null,
        signal: options && options.signal,
        aborter: null,
        abortListener: null
    };
    if (state.signal &&
        (typeof state.signal !== 'object' ||
         typeof state.signal.aborted !== 'boolean' ||
         typeof state.signal.addEventListener !== 'function' ||
         typeof state.signal.removeEventListener !== 'function')) {
        throw new TypeError('signal must be an AbortSignal');
    }
    state.attach = function(aborter) {
        state.aborter = aborter;
        if (state.signal) {
            state.abortListener = function() {
                state.cancelled = true;
                if (state.timer !== null) {
                    clearTimeout(state.timer);
                    state.timer = null;
                }
                state.aborter();
            };
            if (state.signal.aborted) {
                state.abortListener();
                return;
            } else {
                state.signal.addEventListener('abort', state.abortListener, { once: true });
            }
        }
        if (options.timeoutMs > 0) {
            state.timer = setTimeout(function() {
                state.timedOut = true;
                state.aborter();
            }, options.timeoutMs);
        }
    };
    state.cleanup = function() {
        if (state.timer !== null) {
            clearTimeout(state.timer);
            state.timer = null;
        }
        if (state.signal && state.abortListener &&
            typeof state.signal.removeEventListener === 'function') {
            state.signal.removeEventListener('abort', state.abortListener);
        }
    };
    return state;
};

DSN.warn_synchronous_xhr = function() {
    if (DSN.warn_synchronous_xhr.warned) {
        return;
    }
    DSN.warn_synchronous_xhr.warned = true;
    if (typeof console !== 'undefined' && typeof console.warn === 'function') {
        console.warn(
            'Synchronous rDSN XMLHttpRequest calls are deprecated; use <method>Async instead.');
    }
};
DSN.warn_synchronous_xhr.warned = false;

DSN.create_client_class = function(serviceName, methods, generatedTypes) {
    if (!Array.isArray(methods)) {
        throw new TypeError('Generated service methods must be an array');
    }

    var Client = function(website, options) {
        this.url = DSN.normalize_url(website);
        this.options = DSN.merge_options(null, options);
        this.types = DSN.merge_types(generatedTypes, options && options.types);
    };
    Client.serviceName = serviceName;
    Client.prototype.marshall = function(value, type) {
        return marshall_thrift_json(value, type, this.types);
    };
    Client.prototype.unmarshall = function(buffer, value, type) {
        return unmarshall_thrift_json(buffer, value, type, this.types);
    };
    Client.prototype.resolve_type = function(type) {
        return DSN.resolve_type(type, this.types);
    };
    Client.prototype.encode_request = function(value, type) {
        var descriptor = DSN.parse_type(type, this.types);
        return this.marshall(value, descriptor.kind === 'struct' ? 'struct' : type);
    };
    Client.prototype.decode_response = function(buffer, method) {
        if (method.oneWay || method.responseType === 'void' ||
            method.responseType === 'VOID') {
            return null;
        }
        var descriptor = DSN.parse_type(method.responseType, this.types);
        if (descriptor.kind !== 'struct') {
            return this.unmarshall(buffer, null, method.responseType);
        }
        var Type = this.resolve_type(method.responseType);
        if (typeof Type !== 'function') {
            throw new Error('Unknown Thrift response type: ' + method.responseType);
        }
        var value = new Type();
        this.unmarshall(buffer, value, 'struct');
        return value;
    };

    var addMethod = function(method) {
        var syncName = 'internal_' + method.name;
        var asyncName = 'internal_async_' + method.name;
        Client.prototype[syncName] = function(args, hash, callOptions) {
            if (hash && typeof hash === 'object' && callOptions === undefined) {
                callOptions = hash;
                hash = callOptions.hash;
            }
            var self = this;
            var result = null;
            dsn_call(
                this.url,
                method.rpcCode,
                hash,
                'POST',
                this.encode_request(args, method.requestType),
                method.payloadFormat,
                false,
                function(response) {
                    result = self.decode_response(response, method);
                    return result;
                },
                null,
                DSN.merge_options(this.options, callOptions));
            return result;
        };
        Client.prototype[asyncName] = function(
            args, onSuccess, onFail, hash, callOptions) {
            if (hash && typeof hash === 'object' && callOptions === undefined) {
                callOptions = hash;
                hash = callOptions.hash;
            }
            var self = this;
            return dsn_call(
                this.url,
                method.rpcCode,
                hash,
                'POST',
                this.encode_request(args, method.requestType),
                method.payloadFormat,
                true,
                function(response) {
                    var result = self.decode_response(response, method);
                    if (onSuccess) {
                        onSuccess(result);
                    }
                    return result;
                },
                onFail,
                DSN.merge_options(this.options, callOptions));
        };
        Client.prototype[method.name] = function(obj) {
            if (!obj || typeof obj !== 'object') {
                throw new TypeError('RPC call options must be an object');
            }
            var options = DSN.merge_options(obj.options, obj);
            if (!obj.async) {
                return this[syncName](obj.args, obj.hash, options);
            }
            return this[asyncName](
                obj.args, obj.on_success, obj.on_fail, obj.hash, options);
        };
        Client.prototype[method.name + 'Async'] = function(
            args, hashOrOptions, callOptions) {
            if (typeof Promise === 'undefined') {
                throw new Error(
                    'Promises are not supported by this JavaScript environment');
            }
            var self = this;
            var hash = hashOrOptions;
            var options = callOptions;
            if (hashOrOptions && typeof hashOrOptions === 'object') {
                options = hashOrOptions;
                hash = hashOrOptions.hash;
            }
            return new Promise(function(resolve, reject) {
                self[asyncName](
                    args,
                    resolve,
                    function(xhr, textStatus, error) {
                        reject(error);
                    },
                    hash,
                    options);
            });
        };
    };

    for (var methodIndex = 0; methodIndex < methods.length; ++methodIndex) {
        addMethod(methods[methodIndex]);
    }
    return Client;
};

function dsn_call(
    url,
    rpc_code,
    hash,
    method,
    send_data,
    payload_format,
    is_async,
    on_success,
    on_fail,
    options) {
    options = DSN.merge_options(null, options);
    if (!is_async) {
        DSN.warn_synchronous_xhr();
        if (options.timeoutMs > 0 || options.signal) {
            throw new TypeError(
                'timeoutMs and signal are only supported for asynchronous RPC calls');
        }
    }
    if (hash == undefined)
        hash = 0;        
    if (!method)
        method = "POST";

    url = DSN.build_rpc_url(url, payload_format, hash, rpc_code);
    var headers = DSN.build_headers(options);
    var requestState = DSN.create_request_state(options);

    var handle_success = function(response) {
        if (on_success) {
            return on_success(response);
        }
        return response;
    };

    var handle_fail = function(xhr, textStatus, errorThrown) {
        var error = DSN.normalize_rpc_error(
            xhr, textStatus, errorThrown, requestState);
        if (on_fail) {
            on_fail(xhr, textStatus, error);
            return null;
        }
        throw error;
    };

    if (options.signal && options.signal.aborted) {
        requestState.cancelled = true;
        return handle_fail(null, 'abort', new Error('rDSN RPC was cancelled'));
    }

    var jq = (typeof $ !== 'undefined' && $.ajax) ? $ :
             (typeof jQuery !== 'undefined' && jQuery.ajax) ? jQuery : null;
    if (jq && !options.fetch) {
        var jqHeaders = {};
        DSN.copy_headers(jqHeaders, headers);
        DSN.set_header(jqHeaders, 'Content-Type', null);
        var configuredContentType =
            DSN.find_header(options.headers, 'Content-Type');
        var jqXHR = jq.ajax({
            type: method,
            dataType: "text",
            url: url,
            headers: jqHeaders,
            contentType: configuredContentType.found ?
                (configuredContentType.value === null ?
                    false : configuredContentType.value) : undefined,
            data: send_data,
            async: is_async,
            xhrFields: options.credentials === undefined ? undefined : {
                withCredentials: options.credentials === 'include'
            },
            success: function(response, textStatus, jqXHR) {
                requestState.cleanup();
                var responseError = DSN.get_response_error(jqXHR, response);
                if (responseError) {
                    handle_fail(jqXHR, "server_error", responseError);
                    return;
                }
                try {
                    handle_success(response);
                } catch (error) {
                    handle_fail(jqXHR, "parsererror", error);
                }
            },
            error: function(xhr, textStatus, errorThrown){
                requestState.cleanup();
                handle_fail(xhr, textStatus, errorThrown);
            }
        });
        if (is_async) {
            requestState.attach(function() {
                jqXHR.abort();
            });
            if (typeof jqXHR.always === 'function') {
                jqXHR.always(requestState.cleanup);
            }
        }
        return jqXHR;
    }

    var fetchImpl = options.fetch ||
        (dsn_global && typeof dsn_global.fetch === 'function' ?
            dsn_global.fetch.bind(dsn_global) : null);
    if (is_async && fetchImpl) {
        var fetchOptions = {
            method: method,
            headers: headers,
            body: send_data
        };
        if (options.credentials !== undefined) {
            fetchOptions.credentials = options.credentials;
        }
        if (options.timeoutMs > 0 || options.signal) {
            if (typeof AbortController === 'undefined') {
                requestState.cleanup();
                return handle_fail(
                    null,
                    'error',
                    new Error(
                        'AbortController is required for fetch timeout or cancellation'));
            }
            var controller = new AbortController();
            fetchOptions.signal = controller.signal;
            requestState.attach(function() {
                controller.abort();
            });
        }
        var fetchRequest;
        try {
            fetchRequest = fetchImpl(url, fetchOptions);
        } catch (error) {
            requestState.cleanup();
            return handle_fail(null, 'error', error);
        }
        if (!fetchRequest || typeof fetchRequest.then !== 'function') {
            requestState.cleanup();
            return handle_fail(
                null, 'error', new TypeError('fetch must return a Promise'));
        }
        return fetchRequest.then(function(response) {
            return response.text().then(function(responseText) {
                var responseError = DSN.get_response_error(response, responseText);
                if (responseError) {
                    throw responseError;
                }
                return responseText;
            });
        }).then(handle_success).catch(function(error) {
            return handle_fail(error.response || null, "error", error);
        }).then(function(result) {
            requestState.cleanup();
            return result;
        }, function(error) {
            requestState.cleanup();
            throw error;
        });
    }

    if (typeof XMLHttpRequest !== 'undefined') {
        var xhr = new XMLHttpRequest();
        xhr.open(method, url, !!is_async);
        if (options.credentials === 'include') {
            xhr.withCredentials = true;
        } else if (options.credentials === 'omit') {
            xhr.withCredentials = false;
        }
        for (var headerName in headers) {
            if (Object.prototype.hasOwnProperty.call(headers, headerName)) {
                xhr.setRequestHeader(headerName, headers[headerName]);
            }
        }

        var complete_xhr = function() {
            requestState.cleanup();
            var responseError = DSN.get_response_error(xhr, xhr.responseText);
            if (responseError) {
                return handle_fail(xhr, "server_error", responseError);
            }
            try {
                return handle_success(xhr.responseText);
            } catch (error) {
                return handle_fail(xhr, "parsererror", error);
            }
        };

        if (is_async) {
            var xhrComplete = false;
            xhr.onload = function() {
                if (xhrComplete) return;
                xhrComplete = true;
                complete_xhr();
            };
            xhr.onerror = function() {
                if (xhrComplete) return;
                xhrComplete = true;
                requestState.cleanup();
                handle_fail(xhr, 'error', new Error('rDSN RPC transport failed'));
            };
            xhr.onabort = function() {
                if (xhrComplete) return;
                xhrComplete = true;
                requestState.cleanup();
                handle_fail(xhr, 'abort', new Error('rDSN RPC was aborted'));
            };
            xhr.send(send_data);
            requestState.attach(function() {
                xhr.abort();
            });
            return xhr;
        }

        xhr.send(send_data);
        return complete_xhr();
    }

    requestState.cleanup();
    return handle_fail(
        null, 'error', new Error('No HTTP transport available for dsn_call'));
}

function marshall_json_internal(value, type, protocol, registry)
{
    var descriptor = DSN.parse_type(type, registry);

    protocol.writeStructBegin("args");
    protocol.writeFieldBegin('args', descriptor.thrift_type, 0);
    DSN.write_value(value, descriptor, protocol);
    protocol.writeFieldEnd();
    protocol.writeFieldStop();
    protocol.writeStructEnd();
}

function marshall_thrift_json(value, type, registry)
{
    var transport = new Thrift.DSNTransport();
    var protocol  = new Thrift.TJSONProtocol(transport);
    marshall_json_internal(value, type, protocol, registry);
    transport.write(protocol.tstack.pop());
    return transport.getSendBuffer();
}

function dsn_parse_json(buffer)
{
    return Thrift.Int64.parseJSON(buffer);
}

function unmarshall_thrift_internal(value, type, protocol, registry)
{
    var descriptor = DSN.parse_type(type, registry);

    protocol.rstack = [];
    protocol.rpos = [];
    protocol.robj = dsn_parse_json(protocol.transport.readAll());
    protocol.rstack.push(protocol.robj);
    
    protocol.readStructBegin();
    while (true)
    {
        var ret = protocol.readFieldBegin();
        var fname = ret.fname;
        var ftype = ret.ftype;
        var fid = ret.fid;
        if (ftype == Thrift.Type.STOP) {
            break;
        }
        switch (fid)
        {
            case 0:
            if (ftype == descriptor.thrift_type ||
                (descriptor.kind == 'map' && ftype == Thrift.Type.STRUCT))
            {
                value = DSN.read_value(protocol, descriptor, value);
            } else {
                protocol.skip(ftype);
            }
            break;
            default:
                protocol.skip(ftype);
        }
        protocol.readFieldEnd();
    }
    protocol.readStructEnd();
    if (descriptor.kind == "struct")
    {
        /* struct is reference type */
        return null;
    }
    else
    {
        return value;
    }
}

function unmarshall_thrift_json(buffer, value, type, registry)
{
    var transport = new Thrift.DSNTransport(buffer);
    var protocol  = new Thrift.TJSONProtocol(transport);
    return unmarshall_thrift_internal(value, type, protocol, registry);
}

var dsn_transport_exports = {
    Thrift: Thrift,
    DSN: DSN,
    DsnRpcError: DSN.DsnRpcError,
    dsn_call: dsn_call,
    marshall_thrift_json: marshall_thrift_json,
    unmarshall_thrift_json: unmarshall_thrift_json
};

if (dsn_commonjs) {
    module.exports = dsn_transport_exports;
} else if (dsn_global) {
    dsn_global.DSN = DSN;
    dsn_global.DsnRpcError = DSN.DsnRpcError;
    dsn_global.dsn_call = dsn_call;
    dsn_global.marshall_thrift_json = marshall_thrift_json;
    dsn_global.unmarshall_thrift_json = unmarshall_thrift_json;
}
