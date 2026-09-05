#!/bin/bash
set -euo pipefail

SAVES_DIR="/home/steam/.local/share/7DaysToDie/Saves"
RETAIN="${BACKUP_RETAIN:-7}"

while [ $# -gt 0 ]; do
    case "$1" in
        --retain)
            RETAIN="$2"
            shift 2
            ;;
        *)
            echo "[backup] Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [ -z "${S3_BACKUP_BUCKET:-}" ]; then
    echo "[backup] S3_BACKUP_BUCKET not set, skipping."
    exit 0
fi

TIMESTAMP="$(date -u +%Y-%m-%dT%H%M%SZ)"
DEST="s3://${S3_BACKUP_BUCKET}/backups/${TIMESTAMP}/"

echo "[backup] Syncing saves to ${DEST} ..."
aws s3 sync "$SAVES_DIR/" "$DEST" --delete
echo "[backup] Done."

echo "[backup] Pruning to last ${RETAIN} backups..."
PREFIXES="$(aws s3api list-objects-v2 \
    --bucket "$S3_BACKUP_BUCKET" \
    --prefix "backups/" \
    --delimiter "/" \
    --query 'CommonPrefixes[].Prefix' \
    --output text 2>/dev/null || true)"

if [ -n "$PREFIXES" ] && [ "$PREFIXES" != "None" ]; then
    echo "$PREFIXES" \
        | tr '\t' '\n' \
        | sort \
        | head -n -"$RETAIN" \
        | while read -r prefix; do
            [ -n "$prefix" ] || continue
            echo "[backup] Removing old backup: $prefix"
            aws s3 rm "s3://${S3_BACKUP_BUCKET}/${prefix}" --recursive
        done
fi

echo "[backup] Retention complete."
