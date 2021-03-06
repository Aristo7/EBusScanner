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
#include <cassert>
namespace fs = std::filesystem;

class Parser
{
public:
    Parser() = default;

    std::map<std::string, std::vector<std::string>> m_ebus_to_files_usages;
    std::map<std::string, std::vector<std::string>> m_ebus_to_files_connects;

    std::vector<std::string> m_blacklistNamespaces = {"AZ__", "AzFramework__", "LmbrCentral__", "TickBus", "TransformBus", "//", "*"};

    enum class UseType
    {
        Uses,
        Connects,
    };

    bool passes_blacklist(const std::string& name)
    {
        for (const auto& blacklistName : m_blacklistNamespaces)
        {
            if (name.find(blacklistName) != std::string::npos)
            {
                return false;
            }
        }

        return true;
    }

    void add_ebus_usage(const std::string& ebus, const std::string& path, UseType useType)
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

        if (passes_blacklist(ebus))
        {
            // get rid of namespace
            const auto last_column = ebus.find_last_of(":");
            string modified_ebus_name = ebus;
            if (last_column != string::npos)
            {
                modified_ebus_name = ebus.substr(last_column + 1);
            }

            switch (useType)
            {
            case UseType::Uses:
                m_ebus_to_files_usages[modified_ebus_name].push_back(last_token);
                break;
            case UseType::Connects:
                m_ebus_to_files_connects[modified_ebus_name].push_back(last_token);
                break;
            default: ;
                assert(false);
            }
        }
    }

    void check_for(const std::string& pattern, const std::string& one_line, const fs::path& path, UseType useType)
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
            add_ebus_usage(last_token, path.string(), useType);
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
            check_for("::Event(", one_line, path, UseType::Uses);
            check_for("::EventResult(", one_line, path, UseType::Uses);
            check_for("::Broadcast(", one_line, path, UseType::Uses);
            check_for("::BroadcastResult(", one_line, path, UseType::Uses);

            check_for("::Handler::BusConnect(", one_line, path, UseType::Connects);
            check_for("::MultiHandler::BusConnect(", one_line, path, UseType::Connects);
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
        // digraph {
        //   a -> b
        //   a -> b
        //   b -> a [color=blue]
        // }

        using namespace std;
        fstream fs;
        fs.open(out_path, std::fstream::out | std::fstream::trunc);

        fs << "digraph { " << endl;

        const string ebus_style = " [shape=box,style=filled,color=\".7 .3 1.0\"]";

        for (const auto& bus_entry : m_ebus_to_files_usages)
        {
            for (const auto& path : bus_entry.second)
            {
                fs << "    " << bus_entry.first << ebus_style << " ;" << endl;
                fs << "    " << path << " -> " << bus_entry.first << ";" << endl;
            }
        }

        for (const auto& bus_entry : m_ebus_to_files_connects)
        {
            for (const auto& path : bus_entry.second)
            {
                fs << "    " << bus_entry.first << ebus_style << " ;" << endl;
                fs << "    " << bus_entry.first << " -> " << path << ";" << endl;
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
