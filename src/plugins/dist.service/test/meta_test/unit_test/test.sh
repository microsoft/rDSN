#!/bin/sh

if [ -z "${REPORT_DIR}" ]; then
    REPORT_DIR="."
fi

rm -rf core data/ meta_state.dump* zoolog.log 
output_xml="${REPORT_DIR}/dsn.dist.service.meta.test.xml"
GTEST_OUTPUT="xml:${output_xml}" ./dsn.dist.service.meta.test

