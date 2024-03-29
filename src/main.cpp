#include <future>
#include <iostream>
#include <csignal>

#include "utils.h"
#include "Timew.h"
#include "config.h"
#include "Ncurses.h"
#include "sound/AudioPlayer.h"

static constexpr int tmrScreenLines = 2;

static utils::concurrent::queue<PomodoroSession<int64_t, std::nano>> taskQueue;
static std::atomic<bool> isRunning = true, isPause = true;

static auto usr1SigHandler(int) {
    isPause.store(true, std::memory_order::relaxed);
    taskQueue.push({std::chrono::minutes(25), std::chrono::minutes(5), TimewCommand::QUERY});
}

template<typename Rep, typename Period>
static auto countDown(const Ncurses::Screen &tmrScreen, const Ncurses::Screen &cmdScreen, const std::string &title,
                      const std::string &taskDescription, std::chrono::duration<Rep, Period> duration) {
    std::chrono::duration<int64_t, std::nano> delta(0);
    auto prevTime{std::chrono::steady_clock::now()};

    auto running{isRunning.load(std::memory_order::relaxed)}, pause{isPause.load(std::memory_order::relaxed)};
    while (running && !pause && duration.count() > 0) {
        std::string secRep{utils::formatSeconds<Rep, Period>(duration)};
        tmrScreen.putCentered(title, 0, static_cast<int>(title.size()));
        tmrScreen.putCentered(secRep, 1, static_cast<int>(secRep.size()));
        cmdScreen.putCentered(taskDescription, cmdScreen.getLines() - 2, cmdScreen.getCols() - 11);
        auto sleepTime{std::chrono::seconds(1) - delta};
        std::this_thread::sleep_for(sleepTime);
        auto curTime{std::chrono::steady_clock::now()};
        auto timeSlept{curTime - prevTime};
        delta = (timeSlept - sleepTime) % std::chrono::seconds(1);
        duration -= timeSlept;
        prevTime = curTime;
        running = isRunning.load(std::memory_order::relaxed);
        pause = isPause.load(std::memory_order::relaxed);
    }

    return running && !pause;
}

auto main() -> int {
    AudioPlayer audioPlayer;            // handle initialization of audio player
    Ncurses ncurses;                    // handle initialization of ncurses
    Ncurses::Screen cmdScreen(stdscr);
    Ncurses::Screen tmrScreen(tmrScreenLines, COLS, 2, 0);

    std::thread worker([&] {
        audioPlayer.load(PROJECT_INSTALL_PREFIX "/share/" PROJECT_NAME "/sounds/Synth_Brass.ogg");
        audioPlayer.load(PROJECT_INSTALL_PREFIX "/share/" PROJECT_NAME "/sounds/Retro_Synth.ogg");

        while (isRunning.load(std::memory_order_relaxed)) {
            auto task{taskQueue.wait_pop()};
            if (task.timewCommand == TimewCommand::NONE) break;

            isPause.store(false, std::memory_order_relaxed);
            cmdScreen.clear();
            PUT_CENTERED(cmdScreen, "commands: (c)ontinue, (p)ause, (e)xit", 0);

            auto timewQuery = Timew::query();
            std::string taskDescription{std::move(timewQuery.taskDescription)};
            std::chrono::duration<int64_t, std::nano> focusDuration{task.focusDuration};

            try {
                if (task.timewCommand == TimewCommand::RESUME) {
                    if (timewQuery.isTracking) {
                        if (timewQuery.trackedTime > task.focusDuration)
                            focusDuration = std::chrono::duration<long, std::nano>(0);
                        else
                            focusDuration = task.focusDuration - timewQuery.trackedTime;
                    } else {
                        taskDescription = utils::formatDescription(Timew::resume().output);
                    }
                }
            } catch (const std::runtime_error &error) {
                cmdScreen.putFor(error.what(), cmdScreen.getLines() - 1, 0, std::chrono::seconds(2));
                isPause.store(true, std::memory_order_relaxed);
                continue;
            }

            if (!countDown(tmrScreen, cmdScreen, "Focus!", taskDescription, focusDuration)) continue;

            try {
                audioPlayer.play(PROJECT_INSTALL_PREFIX "/share/" PROJECT_NAME "/sounds/Retro_Synth.ogg");
                Timew::stop();
            } catch (const std::runtime_error &error) {
                cmdScreen.putFor(error.what(), cmdScreen.getLines() - 1, 0, std::chrono::seconds(2));
            }

            if (!countDown(tmrScreen, cmdScreen, "Break", taskDescription, task.breakDuration)) continue;

            isPause.store(true, std::memory_order_relaxed);
            audioPlayer.play(PROJECT_INSTALL_PREFIX "/share/" PROJECT_NAME "/sounds/Synth_Brass.ogg");
        }
    });

    struct sigaction sa{.sa_flags = SA_RESTART | SA_NOCLDSTOP};
    sa.sa_handler = usr1SigHandler;
    if (sigaction(SIGUSR1, &sa, nullptr) == EINVAL) {
        cmdScreen.putFor("Unable to handle signals", cmdScreen.getLines() - 1, 0, std::chrono::seconds(1));
    }

    int cmdChar;
    PUT_CENTERED(cmdScreen, "commands: (c)ontinue, (p)ause, (e)xit", 0);
    while ((cmdChar = cmdScreen.getCharToLower()) != 'e') {
        switch (cmdChar) {
            case 'c':
                if (isPause.load(std::memory_order::relaxed)) {
                    taskQueue.push({std::chrono::minutes(25), std::chrono::minutes(5), TimewCommand::RESUME});
                } else {
                    cmdScreen.putFor("Timer is already running", cmdScreen.getLines() - 1, 0, std::chrono::seconds(1));
                }
                break;
            case 'p':
                isPause.store(true, std::memory_order_relaxed);
                try {
                    Timew::stop();
                } catch (const std::runtime_error &error) {}
                break;
            case KEY_RESIZE:
                int lines, cols;
                getmaxyx(stdscr, lines, cols);
                cmdScreen.resize(lines, cols);
                tmrScreen.resize(tmrScreenLines, cols);
                PUT_CENTERED(cmdScreen, "commands: (c)ontinue, (p)ause, (e)xit", 0);
                break;
            default:
                break;
        }
        flushinp();
    }

    isRunning.store(false, std::memory_order_relaxed);  // not used for synchronization
    taskQueue.push({});    // necessary since the thread waits on the queue
    worker.join();

    return 0;
}
