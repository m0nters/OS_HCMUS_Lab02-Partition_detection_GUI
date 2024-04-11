# Partition detection
Manually read FAT32 and NTFS file systems by inspecting the bytes within the system, extract the relevant information, and present it using a graphical user interface (GUI). 

It was assigned as Lab02 project in my Operating System class.

## Table of Contents
- [Project Description](#project-description)
- [Programming langauge and GUI platform we use](#programming-langauge-and-GUI-platform-we-use)
- [Existing errors](#existing-errors)
- [Installation](#installation)
- [Usage](#usage)
- [License](#license)

## Project Description
Assuming a memory card or a USB is divided into at least 2 partitions, in which:
- One partition is formatted in FAT32 format.
- One partition is formatted in NTFS format.

The program should have a graphical user interface (GUI) and the following functions:
1. Automatically detect the format of each partition (FAT32 / NTFS).
2. Display the directory tree of each partition when selected.
3. The display name of the folder/file on the directory tree is the full name (Long File Name).
4. The folder can be collapsed/expanded by user interaction.
5. When selecting any folder/file, read and display information for the user, including:
  - Name (including extension if any)
  - Attributes
  - Date created
  - Time created
  - Total Size

### Bonus points
- **Max +0.5 points** in the final pratical grade if the group implements additional functions:
	- Delete and Restore folder/files
	- Displaying the content of the selected file (text format only)
	- Any other relevant functionalities
## Programming langauge and GUI platform we use
`C++` (both backend and frontend), `Qt Framework`

## Existing errors
Since the project is now closed (the deadline has passed, and we have proceeded with the interview – achieve perfect score with bonus later btw, this shit is easy af), no further work will be conducted on this. 

I believe it would be advantageous to document all current errors in this application version. Doing so will aid future developers (or HCMUS 2nd year students) in continuing to address these issues during the next stage of development (now it's your turn).

---

**Regarding the `Qt` framework (can't fix, blame `Qt` developers):**
- When dragging the splitter between columns in the tree, if the width is too narrow, the icon will overlap the splitter.

**NTFS:**
- Some folders do not display all sub-files (due to excessive data run).
- Unable to display read-only attribute.
- Due to time limitation, we didn't implementation file deletion/restoration feature for this partition typer

**FAT32:**
- Short file/folder names automatically convert to uppercase (even though they were created with lower case characters); however, long file names do not encounter this issue.
- Exist some cases where the content (text) of `.txt` files becomes mess up due to the consequences of previous file deletions without restoration, resulting in the current file's memory region being overwritten by the previous `.txt` file's memory region.
- Can only restore a deleted file within the same application session; thus, if an item is deleted, the application is closed, and then reopened, restoration is no longer possible.
- Unable to restore two items with identical names (e.g., two folders named `A` at different locations in the tree are deleted simultaneously) because the system's item restoration function only accepts two parameters: `<drive number where it resides, item name>`.
- When deleting files, the total size is not updated.
 
	(**Explain**: Basically, when deleting a file, because we only edit the first byte of the related entries in RDET to `E5`, it just disappears from the computer but still occupies memory, that memory area becomes junk memory (there is no way to find it again) and thereby causing the 2nd error. We can only fix this by either restoring the file, then going to Windows Explorer to delete it (not using our software) or reformatting the drive.)

**Mutual**
- Unicode (Vietnamese, Chinese,...) file/folder names experience font errors (although reading `.txt` files with Unicode  characters does not pose any issues).


## Installation
We have compiled the source code into an executable portable application. You can download it from [here](https://www.dropbox.com/scl/fo/ez8wuzl96v4mcldf3ux4b/AIEkasWMcicuvrcVzvEdORE?rlkey=4hybis9fd1p8pgxcxp9qnlwpi&dl=0).

## Usage

Plug in your USB, make sure you have divided it into several partitions before (we only support 2 types: NTFS, FAT32), then run the program.

## License
That's bullshit, we don't care, take it if you want. (the whole program is already a mess btw, hope you can understand the source code)
