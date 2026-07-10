<?php
require_once($argv[1]); // type.php
require_once($argv[2]); // program.php

function collect_js_type_programs($program, &$programs, &$seen)
{
    foreach ($program->includes as $included)
    {
        collect_js_type_programs($included, $programs, $seen);
    }
    if ($program->name === "dsn" || array_key_exists($program->name, $seen))
    {
        return;
    }
    $seen[$program->name] = true;
    $programs[] = $program;
}

function js_type_exports($program)
{
    $exports = array();
    foreach ($program->enums as $enum)
    {
        $exports[] = thelpers::get_js_type_name($enum->name);
    }
    foreach ($program->structs as $struct)
    {
        $exports[] = thelpers::get_js_type_name($struct->name);
    }
    return $exports;
}

$programs = array();
$seen = array();
collect_js_type_programs($_PROG, $programs, $seen);
$modules = array();
foreach ($programs as $program)
{
    $modules[] = array(
        "file" => $program->name."_types.js",
        "exports" => js_type_exports($program)
    );
}
?>
"use strict";

const { loadGeneratedTypeModules } = require("rdsn-client/node");

module.exports = loadGeneratedTypeModules(
    __dirname,
    <?=json_encode($modules, JSON_PRETTY_PRINT)?>);
