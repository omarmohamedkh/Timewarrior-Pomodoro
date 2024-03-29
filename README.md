# Pomodoro Timer for TimeWarrior


Pomodoro timer is a timer interface executing `timew continue` and `timew stop`
commands to mimic pomodoro sessions with
[timewarrior](https://github.com/GothenburgBitFactory/timewarrior) as the backend.

This is intended to be used with
[taskwarrior](https://github.com/GothenburgBitFactory/taskwarrior) and the hook
script that integrates it with
[timewarrior](https://github.com/GothenburgBitFactory/timewarrior). You can learn
more information in their docs.

## Requirements

- ncurses
- vorbis (For desktop)
- OpenAL (For desktop)
- OpenSLES (For embedded devices)

## Installation

```bash
git clone https://github.com/OmarMohamedKhallaf/Timewarrior-Pomodoro.git pomo
cd pomo
# Use -DCMAKE_INSTALL_PREFIX=$PREFIX for termux on android
cmake -B build -S ./ -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4 --target install
```

## Tasks list

- [x] Add sounds after at the end of work and break sessions
- [x] Parse output from child process
- [x] Adapt to changes in terminal size
- [x] Support unicode
- [x] Automatic session tracking by handling signals from taskwarrior hook scripts (Doesn't work in termux )
- [x] Handle text wrapping properly
- [x] Indicate whether it's a focus time or break time in the interface
- [x] Handle errors properly
- [ ] Confirm exit before exiting
- [ ] Make variables configurable
- [ ] Support for `timew start <tags...>` in the interface
- [ ] Use ascii art to print digits adapted to the size of the terminal

## Usage

There are only three commands for now

```text
c: to continue a session followed by a break (must be started by taskwarrior & the hook script)
p: to pause the current session (actually stops it in timewarrior terms)
e: to exit
```

There is a hook scrip that automatically starts tracking by sending a USR1 signal to the program.
This scrip must be executed after timewarrior hook script, to enforce this ordering they must be named in a
lexicological order.

For example:

```text
$ ls -1 ~/.task/hooks/
on-modify.00-timewarrior
on-modify.99-tw-pomodoro
```

So a normal workflow is like this:

- you have your tasks stored in taskwarrior
- you have tw-pomodoro opened in another window, pane, ...
- you `task 1 start` and tw-pomodoro automatically starts the timer and notifies you at the end of sessions
- after a break you press `c` and start your next session

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

[GNU General Public License v3.0](https://choosealicense.com/licenses/gpl-3.0/)
