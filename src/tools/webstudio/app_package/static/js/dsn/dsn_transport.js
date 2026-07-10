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

DSN.resolve_type = function(name) {
    var root;
    if (typeof globalThis !== 'undefined') {
        root = globalThis;
    } else if (typeof window !== 'undefined') {
        root = window;
    } else if (typeof self !== 'undefined') {
        root = self;
    } else if (typeof global !== 'undefined') {
        root = global;
    } else {
        return undefined;
    }

    var normalized = name.replace(/::/g, '.').replace(/^\.+/, '');
    var parts = normalized.split('.');
    var value = root;
    for (var i = 0; i < parts.length && value !== undefined; ++i) {
        value = value[parts[i]];
    }
    if (value !== undefined) {
        return value;
    }
    return root[parts[parts.length - 1]];
};

DSN.parse_type = function(type) {
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
                element: DSN.parse_type(inner)
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
                key: DSN.parse_type(inner.substring(0, comma)),
                value: DSN.parse_type(inner.substring(comma + 1))
            };
        }
    }

    var resolved = DSN.resolve_type(name);
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
    return { kind: 'struct', thrift_type: Thrift.Type.STRUCT, name: name };
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
            if (!(items instanceof Array)) {
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
                var constructor = DSN.resolve_type(descriptor.name);
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

function dsn_call(url, rpc_code, hash, method, send_data, payload_format, is_async, on_success, on_fail) {
    if (url === undefined || url === '') {
        return null;
    }    
    if (hash == undefined)
        hash = 0;        
    if (!method)
        method = "POST";
    
    url = url + "/" + payload_format + "/" + hash + "/" + rpc_code;

    var handle_success = function(response) {
        if (on_success) {
            on_success(response);
        }
        return response;
    };

    var handle_fail = function(xhr, textStatus, errorThrown) {
        if (on_fail) {
            on_fail(xhr, textStatus, errorThrown);
            return null;
        }
        if (is_async) {
            throw (errorThrown || new Error(textStatus || 'dsn_call failed'));
        }
        return null;
    };

    var jq = (typeof $ !== 'undefined' && $.ajax) ? $ :
             (typeof jQuery !== 'undefined' && jQuery.ajax) ? jQuery : null;
    if (jq) {
        return jq.ajax({
            type: method,
            dataType: "text",
            url: url,
            /* 
            the following does not work due to cross-domain queries, see
            http://stackoverflow.com/questions/8538319/jquery-ajax-custom-http-headers-issue
            we therefore encode the url instead as shown above.
            
            headers : {
                'rpc_name' : rpc_code,
                'client_hash' : hash,
                'serialize_format' : payload_format
            }, */
            data: send_data,
            async: is_async,
            success: function(response) {
                handle_success(response);
            },
            error: function(xhr, textStatus, errorThrown){
                handle_fail(xhr, textStatus, errorThrown);
            }
        });
    }

    if (is_async && typeof fetch === 'function') {
        return fetch(url, {
            method: method,
            headers: {
                "Accept": "text/plain, application/json, */*",
                "Content-Type": "application/vnd.apache.thrift.json; charset=utf-8"
            },
            body: send_data
        }).then(function(response) {
            if (!response.ok) {
                var error = new Error('dsn_call failed with HTTP status ' + response.status);
                error.response = response;
                throw error;
            }
            return response.text();
        }).then(handle_success, function(error) {
            if (on_fail) {
                on_fail(null, 'error', error);
                return null;
            }
            throw error;
        });
    }

    if (typeof XMLHttpRequest !== 'undefined') {
        var xhr = new XMLHttpRequest();
        xhr.open(method, url, !!is_async);
        xhr.setRequestHeader("Accept", "text/plain, application/json, */*");
        xhr.setRequestHeader("Content-Type", "application/vnd.apache.thrift.json; charset=utf-8");

        if (is_async) {
            xhr.onreadystatechange = function() {
                if (xhr.readyState !== 4) {
                    return;
                }
                if (xhr.status >= 200 && xhr.status < 300) {
                    handle_success(xhr.responseText);
                } else {
                    handle_fail(xhr, xhr.statusText, new Error('dsn_call failed with HTTP status ' + xhr.status));
                }
            };
            xhr.send(send_data);
            return xhr;
        }

        xhr.send(send_data);
        if (xhr.status >= 200 && xhr.status < 300) {
            return handle_success(xhr.responseText);
        }
        return handle_fail(xhr, xhr.statusText, new Error('dsn_call failed with HTTP status ' + xhr.status));
    }

    throw new Error('No HTTP transport available for dsn_call');
}

function marshall_json_internal(value, type, protocol)
{
    var descriptor = DSN.parse_type(type);

    protocol.writeStructBegin("args");
    protocol.writeFieldBegin('args', descriptor.thrift_type, 0);
    DSN.write_value(value, descriptor, protocol);
    protocol.writeFieldEnd();
    protocol.writeFieldStop();
    protocol.writeStructEnd();
}

function marshall_thrift_json(value, type)
{
    var transport = new Thrift.DSNTransport();
    var protocol  = new Thrift.TJSONProtocol(transport);
    marshall_json_internal(value, type, protocol);
    transport.write(protocol.tstack.pop());
    return transport.getSendBuffer();
}

function dsn_parse_json(buffer)
{
    return Thrift.Int64.parseJSON(buffer);
}

function unmarshall_thrift_internal(value, type, protocol)
{
    var descriptor = DSN.parse_type(type);

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

function unmarshall_thrift_json(buffer, value, type)
{
    var transport = new Thrift.DSNTransport(buffer);
    var protocol  = new Thrift.TJSONProtocol(transport);
    return unmarshall_thrift_internal(value, type, protocol);
}
