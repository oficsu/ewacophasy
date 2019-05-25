This is a simple command-line program for recursively downloading comic strip images from local html sources

### How to use:
After its running, edit settings in displayed file:
* exported_channel_directory — local html source directory (for example, export of Telegram channel, for example t.me/by_duran)
* save_directory — root directory. Each file will be saved in a subdirectory with the name of the comic.
* user_agent — use a specific User-Agent when parsing and downloading
* recursion_depth — follow links no more than specified
* maximun_threads_count — for multithreaded download

### How to build:
Open a project file with Qt Creator or just qmake, and then build
