# Cinder

Delete installed software in a batch, or install CIA files from the SD card, on a Nintendo 3DS.

Cinder lists titles on the SD card and on NAND, lets you mark the ones you want gone, and deletes that batch with `AM_DeleteUserTitles`. System titles are not selectable. The console also refuses them. Cinder will not delete its own title id (`0004000000C1D400`) or the title id of the process that launched it.

Tickets are kept unless you turn that on at the confirm screen. Save data and extra data can remain after the software itself is removed.

Install mode reads `sdmc:/cias/` and the root of the SD card. A CIA is installable when the file size matches the size declared by its header, so a download that stopped early is left alone. That is the only check. Homebrew is not skipped for looking like system software, and a game is not skipped because Nintendo published it. DSiWare goes to NAND. System titles go to NAND. Everything else goes to the SD card. A title that is already installed is replaced.

This needs custom firmware (Luma3DS) the same way any homebrew does.

The HOME Menu icon and banner are a cartoon campfire. Both show the version number. The lists are Dear ImGui, drawn through a citro2d backend, in Pop Happiness. The banner plays a later part of the track, lifted so the average level is much louder, then fades out. It does not fade in. 3.6 uses the same title id as 2.6 (`0004000000C1D400`), so it replaces that icon. 2.5 stays installed and can be deleted from the list. The full track plays while Cinder is open and stops when you leave. Closing the lid stops the track, and opening it starts the track again. The top screen shows total and free space for the SD card and the NAND.

## Controls

| Button | Delete list | Install list | Delete confirm |
| --- | --- | --- | --- |
| A | Toggle the highlighted title | Toggle a complete CIA | Delete |
| B | | Back | Cancel |
| X | Toggle select-all for this list | Toggle select-all for complete CIAs | Toggle ticket deletion |
| Y | | | Toggle matching updates and DLC |
| L / R | Switch the 3DS list and the NDS list | | |
| Select | Switch to install mode | Switch back to the delete list | |
| Start | Open the confirm screen | Open the confirm screen | |
| Start+Select | Exit to the HOME Menu | Exit to the HOME Menu | Exit to the HOME Menu |
| D-pad, Circle Pad | Move | Move | |
| Touch | The 3DS and NDS tabs, or the row under the stylus | The row under the stylus | |

The app opens on the title list. There is no prompt in front of it. Start and Select together close it.

Install rows read the CIA itself: the SMDH icon, the software name, the publisher, the title version, the file size, and the install size (`AM_GetCiaRequiredSpace`, or the content size from the header when that call fails).

On the install confirm screen, A starts the install and B cancels. B during a copy stops the batch. The file already being written is discarded.

The 3DS list is games, demos, updates, and DLC. The NDS list is DSiWare, including titles whose id high is `00030004`. System modules, applets, and system data stay off both lists. A Nintendo game does not. Homebrew that is not one of those system categories does not. Cinder itself is not listed. SD and NAND are both in the list; a row says which one it is.

On the delete confirm screen, "Updates and DLC" adds installed updates (`0004000E`) and DLC (`0004008C`) that share the low title id of a selected game or demo.

## Build

Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the `3ds-dev` packages (`devkitARM`, `libctru`, `3ds-tools`, and for a CIA also `bannertool` / `makerom`).

```sh
cd cinder
make cia
```

That writes `cinder-3.6.cia` and `cinder-3.6.3dsx`. The CIA is signed with developer name Riddle (the SMDH publisher). Install it with FBI or GodMode9. Or copy the 3dsx to `sdmc:/3ds/cinder/` and open it from the Homebrew Launcher. The CIA asks for `am:u`, TWL NAND access, and enough filesystem access to read each title's icon and to install CIAs. The lists are Dear ImGui on citro2d. 3DS icons are the SMDH large icon, drawn at its native 48x48. DSiWare icons are the SRL banner, kept at 32x32 and filtered once when the row is taller. A banner whose flipbook actually changes frames is animated. A static banner stays still. If a name cannot be read, Cinder shows the product code, then the title id.

## What a delete does

1. Marked title ids are checked again. System titles, Cinder, and the host launcher are dropped.
2. Optional: matching updates and DLC on the same media are added.
3. `AM_DeleteUserTitles` removes them in chunks of 16. If a chunk fails, each id is retried with `AM_DeleteUserTitle`.
4. Optional: `AM_DeleteTicket` removes the ticket. A ticket error does not put the software back.

Names come from the installed SMDH (`exefs:/icon`) through the title-content archive, using the system language when that string exists. DSiWare names come from the banner title in the installed SRL.

## What an install does

1. `sdmc:/cias/` is read, then the SD root. Only files whose names end in `.cia` are kept.
2. The CIA header's sections (header, certificates, ticket, TMD, contents, and meta) are padded to 64 bytes and added up. The file is installable when its size is exactly that total. The title id is not a reason to refuse it.
3. The title id comes from `AM_GetCiaFileInfo`. If that title is already installed, Cinder deletes it and its ticket, clears a stuck import, then installs with `AM_StartCiaInstall`. That is what `0xC8E083FC` (already exists) was refusing. DSiWare and system titles go to NAND. Everything else goes to the SD card. Cinder will not delete its own running title.
4. The size is checked again before the first byte is written.