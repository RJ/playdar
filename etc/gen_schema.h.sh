#!/bin/bash
schema=$1
guard="__PLAYDAR_SCHEMA_H__"
if [ -e "$schema" ]
then
cat <<EOF
#ifndef $guard
#define $guard
/*
    This file was automatically generated from $schema on `date`.
*/
namespace playdar {
namespace sql {

static const char * sql = 
EOF
    awk '!/^-/ && length($0) {gsub(/[ \t]+$/, "", $0); gsub("\"","\\\"",$0); printf("\"%s\"\n",$0);}' "$schema"
cat <<EOF
    ;

const char * get_sql()
{
    return sql;
}

}} // namespace

#endif

EOF
else
    echo "Usage: $0 <schema.sql>"
    exit 1
fi
