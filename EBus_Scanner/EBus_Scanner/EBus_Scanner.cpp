// EBus_Scanner.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
namespace fs = std::filesystem;

class Parser
{
public:
    Parser() = default;

    std::map<std::string, std::vector<std::string>> m_ebus_to_files;

    void add_ebus_usage(const std::string& ebus, const std::string& path)
    {
        using namespace std;
        cout << "ebus_scanner found ebus " << ebus << " @ " << path << endl;

        istringstream ss_path(path);

        string one_token;
        while (getline(ss_path, one_token, '\\'))
        {
        }

        string last_token = one_token;

        const auto dot_position = last_token.find(".");
        if (dot_position != string::npos)
        {
            last_token = last_token.substr(0, dot_position);
        }

        size_t pos_column;
        string modified_ebus_name = ebus;
        while ((pos_column = modified_ebus_name.find(":")) != std::string::npos)
        {
            modified_ebus_name.replace(pos_column, 1, "_");
        }

        m_ebus_to_files[modified_ebus_name].push_back(last_token);
    }

    void check_for(const std::string& pattern, const std::string& one_line, const fs::path& path)
    {
        using namespace std;
        const auto position = one_line.find(pattern);
        if (position != string::npos)
        {
            string before_event = one_line.substr(0, position);

            istringstream ss_one_line(before_event);

            string one_token;
            while (getline(ss_one_line, one_token, ' '))
            {
            }

            string last_token = one_token;
            add_ebus_usage(last_token, path.string());
        }
    }

    void parse_file(const fs::path& path)
    {
        using namespace std;

        cout << "ebus_scanner parsing file " << path.string() << endl;
        ifstream t(path);
        string str((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());

        istringstream ss(str);

        string one_line;
        while (getline(ss, one_line, '\n'))
        {
            check_for("::Event(", one_line, path);
            check_for("::EventResult(", one_line, path);
            check_for("::Broadcast(", one_line, path);
            check_for("::BroadcastResult(", one_line, path);
            check_for("::Handler::BusConnect(", one_line, path);
            check_for("::MultiHandler::BusConnect(", one_line, path);
        }
    }

    void parse_path(const std::string& path)
    {
        using namespace std;

        for (const fs::directory_entry& p : fs::directory_iterator(path))
        {
            if (p.is_directory())
            {
                cout << "ebus_scanner found directory " << p.path().string() << endl;
                parse_path(p.path().string());
            }
            else if (p.is_regular_file())
            {
                parse_file(p.path());
            }
        }
    }

    void print_to_dot(const std::string& out_path)
    {
        // strict graph {
        //   a -- b
        //   a -- b
        //   b -- a [color=blue]
        // }

        using namespace std;
        fstream fs;
        fs.open(out_path, std::fstream::out | std::fstream::trunc);

        fs << "strict graph { " << endl;

        for (const auto& bus_entry : m_ebus_to_files)
        {
            for (const auto& path : bus_entry.second)
            {
                fs << "    " << bus_entry.first << " -- " << path << " [constraint=true]; " << endl;
            }
        }

        fs << "}" << endl;

        fs.close();
    }
};


int main(int argc, char** argv)
{
    using namespace std;

    if (argc <= 2)
    {
        cout << "ebus_scanner output.dot path1 path2 ..." << endl;
    }

    Parser p;

    for (int path_i = 2; path_i < argc; ++path_i)
    {
        p.parse_path(string(argv[path_i]));
    }

    p.print_to_dot(std::string(argv[1]));

    return 0;
}
