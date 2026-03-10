#include "TransportationPlannerCommandLine.h"
#include "StringUtils.h"
#include <iomanip>
#include <cmath>

struct CTransportationPlannerCommandLine::SImplementation {
    std::shared_ptr<CDataSource> CmdSrc;
    std::shared_ptr<CDataSink> OutSink;
    std::shared_ptr<CDataSink> ErrSink;
    std::shared_ptr<CDataFactory> Results;
    std::shared_ptr<CTransportationPlanner> Planner;

    std::vector<CTransportationPlanner::TNodeID> LastShortestPath;
    std::vector<CTransportationPlanner::TTripStep> LastFastestPath;
    double LastPathValue = 0.0;
    bool LastWasFastest = false;
    bool HasValidPath = false;

    SImplementation(std::shared_ptr<CDataSource> cmdsrc, std::shared_ptr<CDataSink> outsink, std::shared_ptr<CDataSink> errsink, std::shared_ptr<CDataFactory> results, std::shared_ptr<CTransportationPlanner> planner) {
        CmdSrc = cmdsrc;
        OutSink = outsink;
        ErrSink = errsink;
        Results = results;
        Planner = planner;
    }

    void Write(std::shared_ptr<CDataSink> sink, const std::string &str) {
        // turn string into char and send to sink
        std::vector<char> buffer;
        for (size_t i = 0; i < str.length(); i++) {
            buffer.push_back(str[i]);
        }
        sink->Write(buffer);
    }

    std::string FormatLocation(double lat, double lon) {
        // format lat, N is positive, S is negative
        char NS = 'N'; 
        if (lat < 0) {
            NS = 'S';
        }
        double absLat = std::abs(lat);
        int latD = (int)absLat;
        int latM = (int)((absLat - latD) * 60);
        int latS = (int)std::round(((absLat - latD) * 60 - latM) * 60);
        std::string s1;
        s1 = std::to_string(latD) + "d " + std::to_string(latM) + "' " + std::to_string(latS) + "\" " + NS;

        // format lon, E is positive, W is negative
        char EW = 'E';
        if (lon < 0) {
            EW = 'W';
        }
        double absLon = std::abs(lon);
        int lonD = (int)absLon;
        int lonM = (int)((absLon - lonD) * 60);
        int lonS = (int)std::round(((absLon - lonD) * 60 - lonM) * 60);
        std::string s2;
        s2 = std::to_string(lonD) + "d " + std::to_string(lonM) + "' " + std::to_string(lonS) + "\" " + EW;
        return s1 + ", " + s2;
    }

    // format one number with hour unit into: x hour, x minute, x second
    std::string FormatTime(double hours) {
        int h = (int)hours;
        double remMinutes = (hours - h) * 60;
        int m = (int)remMinutes;
        int s = (int)std::round((remMinutes - m) * 60);

        std::string stime = "";
        if (h > 0) {
            stime = stime + std::to_string(h) + " hr ";
        }
        if (h > 0 || m > 0) {
            stime = stime + std::to_string(m) + " min ";
        }
        if (s > 0 || (h == 0 && m == 0)) {
            stime = stime + std::to_string(s) + " sec.";
        }
        return StringUtils::Strip(stime);
    }

    bool ReadLine(std::string &line) {
        line.clear();
        char c;
        // loop through CmdSrc, store every character in line
        while (CmdSrc->Get(c)) {
            if (c == '\n') return true;
            line += c;
        }
        return !line.empty();
    }

    bool Process() {
        std::string line;
        Write(OutSink, "> ");

        while (ReadLine(line)) {
            std::string cleaned = StringUtils::Strip(line);
            if (cleaned.empty()) {
                Write(OutSink, "> ");
                continue;
            }
            std::vector<std::string> words = StringUtils::Split(cleaned);
            std::string cmd = StringUtils::Lower(words[0]);

            if (cmd == "help") {
                // store menu information
                std::string help_content = "";
                help_content += "------------------------------------------------------------------------\n";
                help_content += "help     Display this help menu\n";
                help_content += "exit     Exit the program\n";
                help_content += "count    Output the number of nodes in the map\n";
                help_content += "node     Syntax \"node [0, count)\" \n";
                help_content += "         Will output node ID and Lat/Lon for node\n";
                help_content += "fastest  Syntax \"fastest start end\" \n";
                help_content += "         Calculates the time for fastest path from start to end\n";
                help_content += "shortest Syntax \"shortest start end\" \n";
                help_content += "         Calculates the distance for the shortest path from start to end\n";
                help_content += "save     Saves the last calculated path to file\n";
                help_content += "print    Prints the steps for the last calculated path\n";

                Write(OutSink, help_content);
            } 

            else if (cmd == "exit") {
                return true; // end loop
            }

            else if (cmd == "count") {
                std::size_t node_num = Planner->NodeCount();
                std:: string result = std::to_string(node_num) + " nodes\n";
                Write(OutSink, result);
            }
            
            else if (cmd == "node") {
                // if not enough argument
                if (words.size() < 2) {
                    Write(ErrSink, "Invalid node command, see help.\n");
                } 

                else {
                    try {
                        std::size_t index = std::stoul(words[1]);
                        size_t node_num = Planner->NodeCount();
                        // check out of bound
                        if (index >= node_num) {
                            Write(ErrSink, "Invalid node parameter, see help.\n");
                        } 
                        else {
                            // use index, get the node, location, and format location
                            auto node = Planner->SortedNodeByIndex(index);
                            auto location = node->Location();
                            std::string formated_loc = FormatLocation(location.DLatitude, location.DLongitude);
                            std::string result = "Node " + std::to_string(index) + ": id = ";
                            result = result + std::to_string(node->ID()) + " is at " + formated_loc + "\n";
                            Write(OutSink, result);
                        }
                    } catch (...) {
                        // if cannot formate, eg not number
                        Write(ErrSink, "Invalid node parameter, see help.\n");
                    }
                }
            }

            else if (cmd == "shortest") {
                // need 3: command, start ID, end ID
                if (words.size() < 3) {
                    Write(ErrSink, "Invalid shortest command, see help.\n");
                }
                else {
                    try {
                        // convert string in argument into unsign long long
                        uint64_t start = std::stoull(words[1]);
                        uint64_t end = std::stoull(words[2]);
                        double d = Planner->FindShortestPath(start, end, LastShortestPath);
                        // if value returned is not -1, means got the distance 
                        if (d != -1.0) {
                            HasValidPath = true; 
                            LastWasFastest = false;
                            LastPathValue = d;
                            char buffer[64];
                            // only keep one decimal 
                            sprintf(buffer, "%.1f", LastPathValue);
                            std::string result = "Shortest path is ";
                            result = result + buffer + " mi.\n";
                            Write(OutSink, result);
                        }
                    } 
                    catch (...) {
                        // error is start / end is not number
                        Write(ErrSink, "Invalid shortest parameter, see help.\n");
                    }
                }
            }
            
            else if (cmd == "fastest") {
                // need 3: command, start ID, end ID
                if (words.size() < 3) {
                    Write(ErrSink, "Invalid fastest command, see help.\n");
                }
                else {
                    try {
                        // convert string from argument into unsign long long
                        uint64_t start = std::stoull(words[1]);
                        uint64_t end = std::stoull(words[2]);
                        double t = Planner->FindFastestPath(start, end, LastFastestPath);
                        // if successfully got a valid time
                        if (t != -1.0) {
                            HasValidPath = true; 
                            LastWasFastest = true;
                            LastPathValue = t;
                            // format the time
                            std::string formated_t = FormatTime(t);
                            std::string result = "Fastest path takes " + formated_t + "\n";
                            Write(OutSink, result);
                        }
                    } 
                    catch (...) {
                        // if start or end is not valid number
                        Write(ErrSink, "Invalid fastest parameter, see help.\n");
                    }
                }
            }

            else if (cmd == "print") {
                // check whether got valid path
                if (HasValidPath == false) {
                    Write(ErrSink, "No valid path to print, see help.\n");
                }
                else {
                    // assign final fastest path into description
                    std::vector<std::string> descriptions;
                    if (Planner->GetPathDescription(LastFastestPath, descriptions)) {
                        // loop to write out
                        for (size_t i = 0; i < descriptions.size(); i++) {
                            std::string line = descriptions[i] + "\n";
                            Write(OutSink, line);
                        }
                    }
                }
            }

            else if (cmd == "save") {
                if (HasValidPath == false) {
                    std::string result = "No valid path to save, see help.\n";
                    Write(ErrSink, result);
                }
                else {
                    // prepare file name
                    // startID_endID_hr.csv
                    std::string startID = words[1];
                    std::string endID = words[2];
                    std::string val_str = std::to_string(LastPathValue); // convert value to string
                    // startID_endID_hr.csv
                    std::string filename = startID + "_" + endID + "_" + val_str + "hr.csv";
                    
                    // build a file sink
                    auto fileSink = Results->CreateSink(filename);

                    if (fileSink) {
                        // file header
                        std::string header = "mode,node_id";
                        Write(fileSink, header);

                        // for loop find every segment
                        int num_steps = LastFastestPath.size();
                        for (int i = 0; i < num_steps; i++) {
                            // get transportation mode
                            auto mode = LastFastestPath[i].first;
                            std::string mode_str;
                            if (mode == CTransportationPlanner::ETransportationMode::Bike) {
                                mode_str = "Bike";
                            } 
                            else if (mode == CTransportationPlanner::ETransportationMode::Bus) {
                                mode_str = "Bus";
                            }
                            else{
                                mode_str = "Walk";
                            }
                            // get node ID
                            std::string node_id = std::to_string(LastFastestPath[i].second);

                            std::string line = "\n" + mode_str + "," + node_id;
                            Write(fileSink, line);
                        }
                        // finish saving
                        std::string success = "Path saved to <results>/" + filename + "\n";
                        Write(OutSink, success);
                    }
                }
            }

            else {
                std::string unknown = "Unknown command \"" + cmd + "\" type help for help.\n";
                Write(ErrSink, unknown);
            }
            Write(OutSink, "> ");
        }
        return true;
    }
};

CTransportationPlannerCommandLine::CTransportationPlannerCommandLine(std::shared_ptr<CDataSource> cmdsrc, std::shared_ptr<CDataSink> outsink, std::shared_ptr<CDataSink> errsink, std::shared_ptr<CDataFactory> results, std::shared_ptr<CTransportationPlanner> planner) {
    DImplementation = std::make_unique<SImplementation>(cmdsrc, outsink, errsink, results, planner);
}

CTransportationPlannerCommandLine::~CTransportationPlannerCommandLine() = default;

bool CTransportationPlannerCommandLine::ProcessCommands() {
    return DImplementation->Process();
}