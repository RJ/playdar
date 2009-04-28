#!/bin/bash
schema=$1
if [ -e "$schema" ]
then
cat <<EOF
/*
    This file was automatically generated from $schema on `date`.
*/
#include <string>

namespace playdar {
namespace sql {

std::string playdar_db()
{
    std::string sql = 
EOF
    awk '!/^-/ && length($0) {gsub("\"","\\\"",$0); printf("\"%s\"\n",$0);}' "$schema"
cat <<EOF
    ;

    return sql;
}

}} // namespace

EOF
else
    echo "Usage: $0 <schema.sql>"
    exit 1
fi
