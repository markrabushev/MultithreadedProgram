#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <string>
#include <windows.h> 
#include <algorithm>
#include <mutex>

std::mutex m;
std::vector < std::pair<std::pair<int, std::chrono::steady_clock::time_point>, std::string>> messages;
bool check_list = false;
std::vector<std::thread> threads;

void StartCount(int delay, std::string message)
{
    std::this_thread::sleep_for(std::chrono::seconds(delay));

    m.lock();
    std::thread::id current_thread_id = std::this_thread::get_id();
    auto it = std::find_if(threads.begin(), threads.end(), [&](const std::thread& t) {
        return t.get_id() == current_thread_id;
        });

    if (it != threads.end()) {
        m.unlock();
        MessageBoxA(0, message.c_str(), "Notification", MB_OK);
    }
    else {
        m.unlock();
        return;
    }
}

void New(int delay, std::string message)
{
    messages.push_back(std::make_pair(std::make_pair(delay, std::chrono::steady_clock::now()), message));
    std::cout << "New notification scheduled!" << std::endl;
    std::lock_guard<std::mutex> guard(m);
    threads.push_back(std::thread(StartCount, delay, message));
}


void List()
{
    if (messages.size() == 0) {
        std::cout << "No active notifications" << std::endl;
        return;
    }
    std::cout << "Scheduled notifications:" << std::endl;
    auto time = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> guard(m);
    for (int i = 0; i < messages.size(); i++) {
        if (messages[i].first.first - std::chrono::duration_cast<std::chrono::seconds>(time - messages[i].first.second).count() <= 0)
        {
            messages.erase(messages.begin() + i);
            threads[i].detach();
            threads.erase(threads.begin() + i);
            i--;
        }
        else {
            std::cout << "    " << i + 1 << ".    " << messages[i].second << " (in " <<
                messages[i].first.first - std::chrono::duration_cast<std::chrono::seconds>(time - messages[i].first.second).count()
                << " seconds)" << std::endl;
        }
    }
    check_list = true;
}

void Cancel(int index)
{
    if (messages.size() == 0 || messages.size() <= index - 1 || check_list == false) {
        return;
    }
    std::cout << "Notification with text \"" << messages[index - 1].second << "\" was cancelled" << std::endl;
    std::lock_guard<std::mutex> guard(m);
    threads[index - 1].detach();
    threads[index - 1] = std::thread([]() {});
    messages[index - 1].first.first = 0;
}

void Exit()
{
    std::lock_guard<std::mutex> guard(m);
    for (auto & t : threads)
    {
        t.detach();
    }
}

enum class CommandType
{
    None,
    New,
    List,
    Cancel,
    Exit
};

struct Command
{
    CommandType type{};
    int delay{};
    std::string message;
    int index{};

    Command() = default;
    Command(CommandType type) : type(type) {}
    Command(CommandType type, int delay, const std::string& message) : type(type), delay(delay), message(message) {}
    Command(CommandType type, int index) : type(type), index(index) {}
};

Command ReadCommand()
{
    std::string input;
    std::getline(std::cin, input);

    std::stringstream parser(input);
    std::string command;
    if (parser >> command)
    {
        if (command == "exit" || command == "x")
            return Command(CommandType::Exit);
        else if (command == "list" || command == "l")
            return Command(CommandType::List);
        else if (command == "cancel" || command == "c")
        {
            int index = 0;
            if (parser >> index)
                return Command(CommandType::Cancel, index);
            else
            {
                std::cerr << "Usage: cancel index" << std::endl
                    << "   index : index of the item to remove" << std::endl;
            }
        }
        else if (command == "new" || command == "n")
        {
            int delay = 0;
            if (parser >> delay)
            {
                if (delay > 0)
                {
                    std::string message;
                    std::getline(parser, message);
                    if (!message.empty())
                        message = message.substr(1);
                    return Command(CommandType::New, delay, message);
                }
                else
                    std::cerr << "Delay must be positive" << std::endl;
            }
            else
            {
                std::cerr << "Usage: new delay message" << std::endl
                    << "   delay   : positive delay in seconds" << std::endl
                    << "   message : message to show without quotes" << std::endl;
            }
        }
        else
        {
            std::cerr << "Unknown command" << std::endl;
        }
    }

    return Command(CommandType::None);
}

int main()
{
    std::cout
        << "Commands:" << std::endl
        << "   new <delay> <message>" << std::endl
        << "      Schedule a notification with given message and delay in seconds" << std::endl
        << "      delay   : positive delay in seconds" << std::endl
        << "      message : message to show without quotes" << std::endl
        << "   list" << std::endl
        << "      Show the list of active notifications" << std::endl
        << "   cancel <index>" << std::endl
        << "      Cancel active notification with given index" << std::endl
        << "      index   : index in the previously shown list" << std::endl
        << "   exit" << std::endl
        << "      Exit application" << std::endl;

    while (true)
    {
        std::cout << std::endl << "> ";
        const auto command = ReadCommand();
        switch (command.type)
        {
        case CommandType::None:
            continue;
        case CommandType::Exit:
            Exit();
            return 0;
        case CommandType::New:
            New(command.delay, command.message);
            break;
        case CommandType::List:
            List();
            break;
        case CommandType::Cancel:
            Cancel(command.index);
            break;
        default:
            assert(0);
        }
    }
    return 0;
}