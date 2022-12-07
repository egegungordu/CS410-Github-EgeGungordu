#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iterator>
#include <queue>
#include <numeric>

// abstract class for a Finite Automaton
class FA {
public:
    void read(std::string filename) {
        std::ifstream file(filename);
        std::string line;
        std::string state;
        std::string symbol;
        std::string next_state;
        std::string type;
        
        if (!file.is_open()) {
            throw std::runtime_error("File not found");
        }

        while (std::getline(file, line)) {
            // remove trailing whitespace
            line.erase(std::remove_if(line.end()-1, line.end(), ::isspace), line.end());
            if (line == "ALPHABET") {
                type = "ALPHABET";
            } else if (line == "STATES") {
                type = "STATES";
            } else if (line == "START") {
                type = "START";
            } else if (line == "FINAL") {
                type = "FINAL";
            } else if (line == "TRANSITIONS") {
                type = "TRANSITIONS";
            } else if (line == "END") {
                break;
            } else {
                if (type == "ALPHABET") {
                    alphabet.insert(line);
                } else if (type == "STATES") {
                    states.insert(line);
                } else if (type == "START") {
                    start = line;
                } else if (type == "FINAL") {
                    final.insert(line);
                } else if (type == "TRANSITIONS") {
                    std::istringstream iss(line);
                    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                                    std::istream_iterator<std::string>{}};
                    state = tokens[0];
                    symbol = tokens[1];
                    next_state = tokens[2];
                    transitions[state][symbol].insert(next_state);
                }
            }
        }

        validate_automaton();
    };
    void print() {
        std::cout << "ALPHABET" << std::endl;
        std::vector<std::string> alphabet_vec(alphabet.begin(), alphabet.end());
        std::sort(alphabet_vec.begin(), alphabet_vec.end());
        for (const auto& symbol : alphabet_vec) {
            std::cout << symbol << std::endl;
        }
        std::cout << "STATES" << std::endl;
        std::vector<std::string> states_vec(states.begin(), states.end());
        std::sort(states_vec.begin(), states_vec.end());
        for (const auto& state : states_vec) {
            std::cout << state << std::endl;
        }
        std::cout << "START" << std::endl;
        if (start != "") {
            std::cout << start << std::endl;
        }
        std::cout << "FINAL" << std::endl;
        std::vector<std::string> final_vec(final.begin(), final.end());
        std::sort(final_vec.begin(), final_vec.end());
        for (const auto& state : final_vec) {
            std::cout << state << std::endl;
        }
        std::cout << "TRANSITIONS" << std::endl;
        for (const auto state : states_vec) {
            for (const auto symbol : alphabet_vec) {
                for (const auto next_state : transitions[state][symbol]) {
                    std::cout << state << " " << symbol << " " << next_state << std::endl;
                }
            }
        }
        std::cout << "END" << std::endl;
    };
    virtual ~FA() {}

protected:
    std::unordered_set<std::string> alphabet;
    std::unordered_set<std::string> states;
    std::string start = "";
    std::unordered_set<std::string> final;
    std::unordered_map<std::string, std::unordered_map<std::string, std::set<std::string>>> transitions;
    virtual void validate_automaton() {
        // check that there is a start state
        if (start == "") {
            throw std::invalid_argument("No start state");
        }
        // check that start state is in states
        if (std::find(states.begin(), states.end(), start) == states.end()) {
            throw std::invalid_argument("Start state " + start + " is not in states");
        }
        // check that all final states are in states
        for (const auto state : final) {
            if (std::find(states.begin(), states.end(), state) == states.end()) {
                throw std::invalid_argument("Final state " + state + " is not in states");
            }
        }

        // check that all states in transitions are in states
        // check that all symbols in transitions are in alphabet
        // check that all next states in transitions are in states
        for (const auto& [state, symbol_map] : transitions) {
            if (std::find(states.begin(), states.end(), state) == states.end()) {
                throw std::invalid_argument("Transition state " + state + " is not in states");
            }
            for (const auto& [symbol, next_state_set] : symbol_map) {
                if (std::find(alphabet.begin(), alphabet.end(), symbol) == alphabet.end()) {
                    throw std::invalid_argument("Transition symbol " + symbol + " is not in alphabet");
                }
                for (const auto next_state : next_state_set) {
                    if (std::find(states.begin(), states.end(), next_state) == states.end()) {
                        throw std::invalid_argument("Transition target state " + next_state + " is not in states");
                    }
                }
            }
        }
    }
    std::string set_to_string(const std::set<std::string>& set) {
        if (set.empty()) {
            return "";
        } else if (set.size() == 1) {
            return *set.begin();
        } else {
            std::string str = "{";
            for (const auto& item : set) {
                str += item + ",";
            }
            str.pop_back();
            str += "}";
            return str;
        }
    }
};

class DFA : public FA {
    friend class NFA;
public:
    DFA() {}
    void validate_automaton() override {
        FA::validate_automaton();
        // check that for each state, there is a transition for each symbol
        // check that for each state, there is at most one transition for each symbol
        for (const auto state : states) {
            for (const auto symbol : alphabet) {
                if (transitions[state][symbol].size() == 0) {
                    throw std::invalid_argument("State " + state + " has no transition for symbol " + symbol);
                } else if (transitions[state][symbol].size() > 1) {
                    throw std::invalid_argument("State " + state + " has more than one transition for symbol " + symbol);
                }
            }
        }
    }
};

// need this hash function for unordered_set of sets
template<typename T>
struct hash_on_sum
: private std::hash<typename T::value_type> {
    typedef typename T::value_type value_type;
    typedef typename std::hash<value_type> base;
    std::size_t operator()(T const& obj) const {
        return base::operator()(std::accumulate(obj.begin(), obj.end(), value_type()));
    }
};

class NFA : public FA {
    friend class DFA;
public:
    NFA() {}
    DFA to_dfa() { 
        DFA dfa;
        std::string sink_state = "SINK";
        dfa.alphabet = alphabet;
        dfa.start = start;
        std::queue<std::set<std::string>> operation_queue;
        std::unordered_set<std::set<std::string>, hash_on_sum<std::set<std::string>>> visited;

        operation_queue.push({start});
        visited.insert({start});
        // while queue is not empty
        while (!operation_queue.empty()) {
            // pop a set of states from the queue
            std::set<std::string> current_states = operation_queue.front();
            operation_queue.pop();
            auto current_state_str = set_to_string(current_states);
            
            // if the set of states is not in dfa.states
            if (dfa.states.find(current_state_str) == dfa.states.end()) {
                // add the set of states to dfa.states
                dfa.states.insert(current_state_str);
            }
            // for each state in current_states
            for (const auto state : current_states) {
                // if state is in final
                if (final.find(state) != final.end()) {
                    // add the set of states to dfa.final
                    dfa.final.insert(current_state_str);
                }
                // for each symbol in alphabet
                for (const auto symbol : alphabet) {
                    // create a set of next states
                    std::set<std::string> next_states;
                    // for each state in current_states
                    for (const auto state : current_states) {
                        // add transitions[state][symbol] to next_states
                        next_states.insert(transitions[state][symbol].begin(), transitions[state][symbol].end());
                    }
                    // if next_states is not empty
                    if (!next_states.empty()) {
                        // add next_states to dfa.transitions
                        auto next_state_str = set_to_string(next_states);
                        dfa.transitions[current_state_str][symbol].insert(next_state_str);
                        // push next_states to the queue
                        if (visited.find(next_states) == visited.end()) {
                            operation_queue.push(next_states);
                            visited.insert(next_states);
                        }
                    } else {
                        // add sink_state to dfa.transitions
                        dfa.states.insert(sink_state);
                        for (const auto symbol : alphabet) {
                            dfa.transitions[sink_state][symbol].insert(sink_state);
                        }
                        dfa.transitions[current_state_str][symbol] = {sink_state};
                    }
                }
            }
        }
        return dfa;
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        // print usage
        auto program_name = std::string(argv[0]);
        auto last_slash = program_name.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            program_name = program_name.substr(last_slash + 1);
        }
        std::cerr << "Usage: ./" << program_name << " <input_file>" << std::endl;
        return 1;
    }
    std::string file_location = argv[1];

    try {
        NFA nfa;
        nfa.read(file_location);
        DFA dfa = nfa.to_dfa(); 
        dfa.print();
    } catch (const std::invalid_argument& e) {
        std::cout << e.what() << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
