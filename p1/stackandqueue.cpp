/*
 * ECE-264 DSA I
 * Daniel Mezhiborsky
 * Prof. Sable - Spring 2021
 * Program 1 - Stacks and Queues
 */

// note to Prof. Sable - I do have some light syntax error checking. It's definitely not complete.

#include <fstream>
#include <iostream>
#include <list>
#include <optional>
#include <sstream>
#include <string>

#include <errno.h>
#include <string.h>

template <typename T>
class BasicList {
    class Node {
        public:
            Node(T c):
                data(c)
                {}
            Node(T c, Node* n):
                data(c), 
                next(n) 
                {}
            T data;
            Node* next = nullptr;
    };
    // by definition, an empty list will have both head and tail be nullptr
    // this means either both are nullptr or neither are nullptr - otherwise error! 
    // if there's a single element, both head and tail should point to it
    Node *head = nullptr, *tail = nullptr;
    void ensure_empty();
    void init_list(const T&);
    protected:
        BasicList<T>(std::string_view n):
            name(n)
            {}
        std::optional<const T> pop_start();
        void push_start(const T&);
        void push_end(const T&);
    public:
        // MyStack and MyQueue don't have any data members at all, but make the ctor virtual regardless
        // in our case, this isn't going to run anyway (bc main() scope, exit doesn't call all dtors, blah)
        virtual ~BasicList() { // maybe more efficient than pop_start() b/c we don't care about the data
            Node* prev = head;
            while (head) {
                head = head->next;
                delete prev;
            }
        };
        const std::string name;
        virtual std::optional<const T> pop() = 0;
        virtual void push(const T&) = 0;
};

// call this on a list that you know is empty as an error check
// checking both head and tail is redundant because the caller
// will have looked at one or the other to determine that the list
// is empty in the first place, but check both here anyway.
template <typename T>
void BasicList<T>::ensure_empty() {
    if (head || tail) { // warning!
        std::cerr << "WARNING: malformed BasicList" << '\n';
        head = nullptr;
        tail = nullptr; // leave it in a good state going forward!
    }
}

// call this to initialize an empty list you know is empty
template <typename T>
void BasicList<T>::init_list(const T& i) {
    ensure_empty();
    head = new Node(i); // next=nullptr (see ctor)
    tail = head;
}

// push to start of list (makes new head)
template <typename T>
void BasicList<T>::push_start(const T& i) {
    if (!head) { // empty list
        init_list(i); // includes sanity check
    } else {
        head = new Node(i, head);
    }
}

// push to end of list (makes new tail)
template <typename T>
void BasicList<T>::push_end(const T& i) {
    if (!head) { // empty list
        init_list(i); // includes sanity check
    } else {
        Node* n = new Node(i); // next=nullptr (see ctor)
        tail->next = n;
        tail = n;
    }
}

// pop from beginning of list
template <typename T>
std::optional<const T> BasicList<T>::pop_start() {
    if (!head) { // empty list
        ensure_empty();
        return {};
    } else {
        // we're about to destroy the node
        T val = std::move(head->data);

        Node* old = head;
        head = head->next;
        if (!head) tail = nullptr;
        delete old;
        
        return val; // copy elision probably
    }
}

// stack uses pop_start() and push_start()
template <typename T>
class MyStack : public BasicList<T> {
    public:
        MyStack<T>(std::string_view n):
            BasicList<T>(n)
            {}
        std::optional<const T> pop() { return BasicList<T>::pop_start(); }
        void push(const T& i) { BasicList<T>::push_start(i); }
        ~MyStack<T>() {}
};

// queue uses pop_start() and push_end()
template <typename T>
class MyQueue : public BasicList<T> {
    public:
        MyQueue<T>(std::string_view n):
            BasicList<T>(n)
            {}
        std::optional<const T> pop() { return BasicList<T>::pop_start(); }
        void push(const T& i) { BasicList<T>::push_end(i); }
        ~MyQueue<T>() {}
};

// ------------------------------------------------------------------------------------------------

// print error to stderr/cerr and skip a line
// no guarantees on this; it's very primitive error recovery
void error_skip(std::string_view w, std::ifstream& f) {
    std::cerr << "Syntax error near word '" << w << "'; skipping line!" << '\n';
    f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// It's annoying to me that you can't use std::optional with references -
// I get the design decision but I personally think it's dumb. A pointer is fine!
// return nullptr if not found
template <typename T>
BasicList<T>* find_list(const std::list<BasicList<T> *>& metalist, std::string_view name) {
    for (auto const& l : metalist) {
        if (l->name == name) {
            return l;
        }
    }
    return nullptr;
}

// lots of parameters - the only reason I'm passing the input file is for my error
// printing function, which is optional
template <typename T>
void do_command(std::list<BasicList<T> *>& metalist, std::string_view cmd, // val is not string_view b/c istringstream
                                  std::string_view name, const std::string& val, std::ofstream& out,
                                  std::ifstream& in) {
    if (cmd == "create") {
        if (!find_list(metalist, name)) {
            if (val == "stack") {
                metalist.push_back(new MyStack<T>(name));
            } else if (val == "queue") {
                metalist.push_back(new MyQueue<T>(name));
            } else {
                error_skip(val, in);
            }
        } else {
            out << "ERROR: This name already exists!" << '\n';
        }

    } else if (cmd == "push") {
        std::istringstream ss(val);
        T nval;
        ss >> nval;
        if (auto target = find_list(metalist, name)) {
            target->push(nval);
        } else {
            out << "ERROR: This name does not exist!" << '\n';
        }

    } else if (cmd == "pop") {
        if (auto target = find_list(metalist, name)) {
            if (auto r = target->pop()) {
                out << "Value popped: " << r.value() << '\n';
            } else {
                out << "ERROR: This list is empty!" << '\n';
            }
        } else {
            out << "ERROR: This name does not exist!" << '\n';
        }
    } else {
        error_skip(cmd, in);
    }
}

// get file path input from the user
std::string get_path(std::string_view file_type) {
    std::string path;
    std::cout << "Please enter the name of the " << file_type << "file: ";
    std::cin >> path;
    return path;
}

// open input/output file with error checking, reporting, and retry
// T should be either std::ifstream or std::ofstream
template <typename T>
T open_file(std::string_view file_type) {
    T f;
    while (true) {
        f = T(get_path(file_type));
        if (f.is_open())
            return f;
        else
            std::cerr << "Error opening " << file_type << " file: " << strerror(errno) << '\n';
    }
}

int main() {
    auto in_file = open_file<std::ifstream>("input");
    auto out_file = open_file<std::ofstream>("output");

    std::list<BasicList<int> *> listSLi;
    std::list<BasicList<double> *> listSLd;
    std::list<BasicList<std::string> *> listSLs;

    // I'm not sure if I regret this split or not. getline + tokenizing separately
    // would have been fine too.
    std::string w1, w2, w3;
    while (in_file >> w1 >> w2) { // (cmd) (T+name)
        out_file << "PROCESSING COMMAND: " << w1 << " " << w2;
        if (in_file.peek() != '\n') { // we have a third word!
            in_file >> w3;
            out_file << " " << w3;
        }
        out_file << '\n';
        if (in_file.peek() != '\n') // we should not have anything after the third word
            error_skip(w3, in_file);

        switch (w2.at(0)) {
            case 'i':
                do_command(listSLi, w1, w2.substr(1), w3, out_file, in_file); break;
            case 'd':
                do_command(listSLd, w1, w2.substr(1), w3, out_file, in_file); break;
            case 's':
                do_command(listSLs, w1, w2.substr(1), w3, out_file, in_file); break;
            default:
                error_skip(w2, in_file);
        }
    }
    // no need to explicitly close the files / RAII
}
