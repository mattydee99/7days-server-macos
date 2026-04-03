#!/bin/bash
set -e

GAME_DIR="/opt/7days"
SAVES_DIR="/home/steam/.local/share/7DaysToDie/Saves"
APP_ID="294420"

backup() {
    if [ -n "$S3_BACKUP_BUCKET" ]; then
        echo "[backup] Syncing saves to s3://$S3_BACKUP_BUCKET/saves/ ..."
        aws s3 sync "$SAVES_DIR/" "s3://$S3_BACKUP_BUCKET/saves/" --delete
        echo "[backup] Done."
    else
        echo "[backup] S3_BACKUP_BUCKET not set, skipping."
    fi
}

shutdown() {
    echo "[shutdown] Caught signal, stopping server..."
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -TERM "$SERVER_PID"
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    backup
    exit 0
}

trap shutdown SIGTERM SIGINT

# Install on first run, update on subsequent runs
if [ ! -f "$GAME_DIR/7DaysToDieServer.x86_64" ]; then
    echo "[steamcmd] First run — installing 7 Days to Die..."
    VALIDATE="validate"
else
    echo "[steamcmd] Checking for updates..."
    VALIDATE=""
fi

/opt/steamcmd/steamcmd.sh \
    +force_install_dir "$GAME_DIR" \
    +login anonymous \
    +app_update "$APP_ID" $VALIDATE \
    +quit

echo "[server] Starting 7 Days to Die..."
"$GAME_DIR/7DaysToDieServer.x86_64" \
    -configfile=/config/serverconfig.xml \
    -logfile /dev/stdout \
    -batchmode \
    -nographics \
    -dedicated &

SERVER_PID=$!
echo "[server] PID $SERVER_PID"
wait "$SERVER_PID"
