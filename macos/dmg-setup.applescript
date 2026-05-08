on run argv
    set image_name to item 1 of argv
    set volume_path to "/Volumes/" & image_name

    -- Create the /Applications symlink so the user can drag the .app
    -- onto a real install target. CPack does not add this for us.
    do shell script "ln -s /Applications " & quoted form of (volume_path & "/Applications")

    -- CPack copies the background image into .background/ but its
    -- final filename is implementation-defined (older CMake renames to
    -- background.tiff; newer keeps the source basename). Discover it
    -- at runtime so the script doesn't wedge on a hardcoded name.
    set bg_filename to do shell script "ls " & quoted form of (volume_path & "/.background/") & " | head -n 1"

    tell application "Finder"
        tell disk image_name
            open
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            -- Window bounds {x1,y1,x2,y2} -> 600x400, matching the background.
            set the bounds of container window to {200, 100, 800, 500}
            set viewOptions to the icon view options of container window
            set arrangement of viewOptions to not arranged
            set icon size of viewOptions to 96
            set background picture of viewOptions to file (".background:" & bg_filename)
            set position of item "richmd.app" of container window to {150, 200}
            set position of item "Applications" of container window to {450, 200}
            close
            open
            update without registering applications
            delay 1
            close
        end tell
    end tell
end run
