<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php
$file_prefix = $argv[3];
$idl_type = $argv[4];
$idl_format = $argv[5];

$serialization_fmt = "DSF";
if ($idl_type == "thrift")
{
    $serialization_fmt = $serialization_fmt."_THRIFT";
} else if ($idl_type == "proto")
{
    $serialization_fmt = $serialization_fmt."_PROTOC";
}
if ($idl_format == "binary")
{
    $serialization_fmt = $serialization_fmt."_BINARY";
} else if ($idl_format == "json")
{
    $serialization_fmt = $serialization_fmt."_JSON";
}
?>
<?php
function generate_request_helper($client, $func, $async, $serialization_fmt)
{
    $request_type = thelpers::resolve_type_aliases($func->get_request_type_name());
    $return_type = thelpers::resolve_type_aliases($func->get_return_type());
?>
<?=$client?>.prototype.internal_<?php if ($async) { echo "async_";} ?><?=$func->name?> = function(args, <?php if ($async) {echo "on_success, on_fail,";} ?> hash) {
    var self = this;
    var ret = null;
    <?php if ($async) { echo "var request = "; } ?>dsn_call(
        this.url,
        "<?php echo $func->get_rpc_code(); ?>",
        hash,
        "POST",
<?php if (thelpers::is_base_type($request_type)) { ?>
        this.marshall(args, "<?=$request_type?>"),
<?php } else { ?>
        this.marshall(args, "struct"),
<?php } ?>
        "<?=$serialization_fmt?>",
        <?php if($async) {echo "true";} else {echo "false";}?>,
        function(result) {
<?php if ($func->is_one_way()) { ?>
            ret = null;
<?php } else if (thelpers::is_base_type($return_type)) { ?>
            ret = self.unmarshall(result, null, "<?=$return_type?>");
<?php } else { ?>
            ret = new <?=thelpers::get_js_type_name($return_type)?>();
            self.unmarshall(result, ret, "struct");
<?php } ?>
<?php if ($async) { ?>
            if (on_success) {
                on_success(ret);
            }
<?php } ?>
        },
        function(xhr, textStatus, errorThrown) {
            ret = null;
<?php if ($async) { ?>
            if (on_fail) {
                on_fail(xhr, textStatus, errorThrown);
            }
<?php } ?>
        }
    );
    return <?php if ($async) { echo "request || ret"; } else { echo "ret"; } ?>;
}

<?php
}
foreach ($_PROG->services as $svc) 
{   
    $client = $svc->name."App";
?>
<?=$client?> = function(website) {
    this.url = website;
}

<?=$client?>.prototype = {};

<?=$client?>.prototype.marshall = function(value, type) {
    return marshall_thrift_json(value, type);
}

<?=$client?>.prototype.unmarshall = function(buf, value, type) {
    return unmarshall_thrift_json(buf, value, type);
}

<?php
    foreach ($svc->functions as $func)
    {
        generate_request_helper($client, $func, false, $serialization_fmt);
        generate_request_helper($client, $func, true, $serialization_fmt);
?>
<?=$client?>.prototype.<?=$func->name?> = function(obj) {
    if (!obj.async) {
        return this.internal_<?=$func->name?>(obj.args, obj.hash);
    } else {
        return this.internal_async_<?=$func->name?>(obj.args, obj.on_success, obj.on_fail, obj.hash);
    }
}

<?php
    }
?>
<?php 
}
?>
