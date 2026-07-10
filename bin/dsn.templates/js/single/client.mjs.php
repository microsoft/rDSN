<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php
$file_prefix = $argv[3];
$idl_type = $argv[4];
$idl_format = $argv[5];

$serialization_fmt = "DSF_".strtoupper($idl_type)."_".strtoupper($idl_format);

function js_mjs_wire_type($type)
{
    $resolved = thelpers::resolve_type_aliases($type);
    return thelpers::is_base_type($resolved)
        ? $resolved
        : thelpers::get_js_type_name($resolved);
}

function js_mjs_method_descriptors($service, $serialization_fmt)
{
    $descriptors = array();
    foreach ($service->functions as $func)
    {
        $descriptors[] = array(
            "name" => $func->name,
            "rpcCode" => $func->get_rpc_code(),
            "requestType" => js_mjs_wire_type($func->get_request_type_name()),
            "responseType" => js_mjs_wire_type($func->get_return_type()),
            "payloadFormat" => $serialization_fmt,
            "oneWay" => $func->is_one_way()
        );
    }
    return $descriptors;
}

function js_mjs_pretty_json($value, $indent)
{
    return str_replace(
        "\n", "\n".str_repeat(" ", $indent), json_encode($value, JSON_PRETTY_PRINT));
}
?>
import { createClientClass } from "rdsn-client";
import types from "./<?=$file_prefix?>.types.cjs";

<?php foreach ($_PROG->services as $svc) {
    $client = $svc->name."App";
?>
export const <?=$client?> = createClientClass(
    <?=json_encode($svc->name)?>,
    <?=js_mjs_pretty_json(js_mjs_method_descriptors($svc, $serialization_fmt), 4)?>,
    types);
<?php } ?>

export default {
<?php foreach ($_PROG->services as $index => $svc) {
    $client = $svc->name."App";
?>
    <?=$client?><?=$index + 1 < count($_PROG->services) ? "," : ""?>
<?php } ?>
};
