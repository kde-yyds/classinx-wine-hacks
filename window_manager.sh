#!/bin/bash

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Raising toolbox window ID: $1 (resolution: $2)" >&2
}

while true; do
    # Find the first square "ClassIn X" toolbox window
    read -r TOOLBOX_ID RESOLUTION <<<$(xwininfo -root -tree | grep 'ClassIn X' | \
        while read -r line; do
            ID=$(echo "$line" | awk '{print $1}')
            GEOM=$(echo "$line" | grep -o '[0-9]\+x[0-9]\++[0-9]\++[0-9]\+')
            WIDTH=$(echo "$GEOM" | cut -d'x' -f1)
            HEIGHT=$(echo "$GEOM" | cut -d'x' -f2 | cut -d'+' -f1)
            if [ "$WIDTH" = "$HEIGHT" ]; then
                echo "$ID $WIDTHx$HEIGHT"
                break
            fi
        done)

    # Find the fullscreen "ClassIn X.exe" Wine window
    FULLSCREEN_ID=$(xwininfo -root -tree | grep 'ClassIn X.exe' | awk '{print $1}' | head -n 1)

    if [ -n "$TOOLBOX_ID" ]; then
        log "$TOOLBOX_ID" "$RESOLUTION"
        xprop -id "$TOOLBOX_ID" -f _NET_WM_WINDOW_TYPE 32a -set _NET_WM_WINDOW_TYPE _NET_WM_WINDOW_TYPE_NORMAL
        xdotool windowraise "$TOOLBOX_ID"
        xdotool windowactivate "$TOOLBOX_ID"
    else
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] No square toolbox window found." >&2
    fi

    sleep 0.5
done
