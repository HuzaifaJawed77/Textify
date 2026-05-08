#include <iostream>
#include <vector>
#include <ctime>
#include <algorithm>
#include <fstream>

using namespace std;

// ─── File Storage Helpers ────────────────────────────────────────────────────

// Append a single log line to a file
void appendToFile(const string& filename, const string& log) {
    ofstream file(filename, ios::app);

    if (file.is_open())
        file << log << "\n";
}

// Load all lines from a file into a vector
bool loadFromFile(const string& filename, vector<string>& history) {
    ifstream file(filename);

    if (!file.is_open())
        return false;

    string line;

    while (getline(file, line))
        if (!line.empty())
            history.push_back(line);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

// Utility to get current time string
string getCurrentTime() {
    time_t now = time(0);
    tm* ltm = localtime(&now);

    char buffer[10];

    strftime(buffer, sizeof(buffer), "%H:%M:%S", ltm);

    return string(buffer);
}

// ─────────────────────────────────────────────────────────────────────────────
// Abstract Message Class
// ─────────────────────────────────────────────────────────────────────────────

class Message {
public:
    virtual string getContent() const = 0;
    virtual ~Message() {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Derived Message Types
// ─────────────────────────────────────────────────────────────────────────────

class Text : public Message {
    string txt;

public:
    Text(string t) : txt(t) {}

    string getContent() const override {
        return "Text: " + txt;
    }
};

class Image : public Message {
    string fileName;

public:
    Image(string name) : fileName(name) {}

    string getContent() const override {
        return "Image: " + fileName + ".jpeg";
    }
};

class Video : public Message {
    string fileName;

public:
    Video(string name) : fileName(name) {}

    string getContent() const override {
        return "Video: " + fileName + ".mp4";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory Pattern
// ─────────────────────────────────────────────────────────────────────────────

class MessageFactory {
public:
    static Message* create(const string& type, const string& data) {

        if (type == "text")
            return new Text(data);

        if (type == "image")
            return new Image(data);

        if (type == "video")
            return new Video(data);

        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Observer Interface
// ─────────────────────────────────────────────────────────────────────────────

class Observer {
public:
    virtual void update(const string& from, Message* msg) = 0;

    virtual string getName() const = 0;

    virtual ~Observer() {}
};

// ─────────────────────────────────────────────────────────────────────────────
// User Class
// ─────────────────────────────────────────────────────────────────────────────

class User : public Observer {

    string name;

    vector<string> personalChatHistory;
    vector<string> groupChatHistory;

    string personalFile() const {
        return name + "_personal.txt";
    }

public:
    User(string n) : name(n) {

        loadFromFile(personalFile(), personalChatHistory);
    }

    string getName() const override {
        return name;
    }

    // Group notification
    void update(const string& from, Message* msg) override {

        string log =
            "[" + getCurrentTime() + "] " +
            from + " to group: " +
            msg->getContent();

        cout << log << endl;

        groupChatHistory.push_back(log);
    }

    // Sender side
    void sendPersonalMessage(const string& receiverName, Message* msg) {

        string log =
            "[" + getCurrentTime() + "] " +
            name + " to " +
            receiverName + ": " +
            msg->getContent();

        personalChatHistory.push_back(log);

        appendToFile(personalFile(), log);
    }

    // Receiver side
    void receivePersonalMessage(const string& from, Message* msg) {

        string log =
            "[" + getCurrentTime() + "] " +
            from + " to " +
            name + ": " +
            msg->getContent();

        cout << log << endl;

        personalChatHistory.push_back(log);

        appendToFile(personalFile(), log);
    }

    void showPersonalHistory() const {

        cout << "\n--- Personal Chat History of "
             << name << " ---\n";

        if (personalChatHistory.empty()) {
            cout << "No personal messages yet.\n";
        }
        else {
            for (auto& log : personalChatHistory)
                cout << log << endl;
        }

        cout << "-----------------------------------------------\n\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Group Class (Subject / Observable)
// ─────────────────────────────────────────────────────────────────────────────

class Group {

    string groupName;

    vector<Observer*> users;

    vector<string> groupChatHistory;

    string adminName;

    string groupFile() const {
        return groupName + "_chat.txt";
    }

    string buildLog(const string& senderName, Message* msg) const {

        return "[" + getCurrentTime() + "] " +
               senderName + " to " +
               groupName + ": " +
               msg->getContent();
    }

public:

    Group(string name, string initialAdmin)
        : groupName(name),
          adminName(initialAdmin)
    {
        loadFromFile(groupFile(), groupChatHistory);

        if (!groupChatHistory.empty()) {

            cout << "[Loaded "
                 << groupChatHistory.size()
                 << " previous group message(s) from file]\n";
        }
    }

    void add(Observer* u) {

        users.push_back(u);

        cout << u->getName()
             << " added to "
             << groupName << endl;
    }

    void remove(const string& username) {

        if (username == adminName) {

            cout << "Cannot remove admin. Assign a new admin first.\n";
            return;
        }

        auto it = remove_if(
            users.begin(),
            users.end(),

            [&](Observer* u) {
                return u->getName() == username;
            }
        );

        if (it != users.end()) {

            users.erase(it, users.end());

            cout << username
                 << " removed from group.\n";
        }
        else {
            cout << username
                 << " not found in group.\n";
        }
    }

    bool isAdmin(const string& name) const {

        return name == adminName;
    }

    bool isMember(const string& name) const {

        for (auto u : users)
            if (u->getName() == name)
                return true;

        return false;
    }

    void makeAdmin(
        const string& requester,
        const string& newAdminName
    ) {

        if (!isAdmin(requester)) {

            cout << requester
                 << " is not the admin!\n";

            return;
        }

        if (!isMember(newAdminName)) {

            cout << newAdminName
                 << " is not in the group!\n";

            return;
        }

        adminName = newAdminName;

        cout << newAdminName
             << " is now the admin of "
             << groupName << "!\n";
    }

    void sendToGroup(Observer* sender, Message* msg) {

        string log =
            buildLog(sender->getName(), msg);

        cout << log << endl;

        groupChatHistory.push_back(log);

        appendToFile(groupFile(), log);

        for (auto u : users) {

            if (u != sender)
                u->update(sender->getName(), msg);
        }
    }

    void showGroupHistory() const {

        cout << "\n--- Group Chat History ("
             << groupName << ") ---\n";

        if (groupChatHistory.empty()) {

            cout << "No group messages yet.\n";
        }
        else {

            for (auto& log : groupChatHistory)
                cout << log << endl;
        }

        cout << "------------------------------------------------\n\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Utility Functions
// ─────────────────────────────────────────────────────────────────────────────

void printTextifyBanner() {

    cout << "==================================================\n";
    cout << "                Welcome to Textify               \n";
    cout << "       Simple Console Messaging Application      \n";
    cout << "==================================================\n";
}

void displayMenu() {

    cout << "--------------------------------------------------\n";

    cout << "Options:\n"
         << "1. Group Message\n"
         << "2. Personal Message\n"
         << "3. View Group Chat History\n"
         << "4. View Personal Chat History\n"
         << "5. Add/Remove User (Admin Only)\n"
         << "6. Make Admin (Admin Only)\n"
         << "7. View Current Group Members\n"
         << "8. Exit\n";

    cout << "Select option (1-8): ";
}

Observer* getUserByName(
    const string& name,
    vector<Observer*>& userList
) {

    for (auto u : userList)
        if (u->getName() == name)
            return u;

    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Group Message
// ─────────────────────────────────────────────────────────────────────────────

void handleGroupMessage(
    Group& group,
    vector<Observer*>& users
) {

    string senderName;
    string type;
    string content;

    cout << "Sender name: ";
    cin >> senderName;

    Observer* sender =
        getUserByName(senderName, users);

    if (!sender) {

        cout << "Invalid sender!\n";
        return;
    }

    cout << "Message type (text/image/video): ";
    cin >> type;

    cin.ignore();

    cout << "Content (text or file name without extension): ";

    getline(cin, content);

    Message* m =
        MessageFactory::create(type, content);

    if (!m) {

        cout << "Invalid message type!\n";
        return;
    }

    group.sendToGroup(sender, m);

    delete m;
}

// ─────────────────────────────────────────────────────────────────────────────
// Personal Message
// ─────────────────────────────────────────────────────────────────────────────

void handlePersonalMessage(vector<Observer*>& users) {

    string senderName;
    string receiverName;
    string type;
    string content;

    cout << "Sender name: ";
    cin >> senderName;

    cout << "Receiver name: ";
    cin >> receiverName;

    Observer* sender =
        getUserByName(senderName, users);

    Observer* receiver =
        getUserByName(receiverName, users);

    User* senderUser =
        dynamic_cast<User*>(sender);

    User* receiverUser =
        dynamic_cast<User*>(receiver);

    if (!senderUser || !receiverUser) {

        cout << "Invalid name(s)!\n";
        return;
    }

    if (senderUser == receiverUser) {

        cout << "Cannot message yourself.\n";
        return;
    }

    cout << "Message type (text/image/video): ";
    cin >> type;

    cin.ignore();

    cout << "Content (text or file name without extension): ";

    getline(cin, content);

    Message* m =
        MessageFactory::create(type, content);

    if (!m) {

        cout << "Invalid message type!\n";
        return;
    }

    senderUser->sendPersonalMessage(
        receiverUser->getName(),
        m
    );

    receiverUser->receivePersonalMessage(
        senderUser->getName(),
        m
    );

    delete m;
}

// ─────────────────────────────────────────────────────────────────────────────
// View Histories
// ─────────────────────────────────────────────────────────────────────────────

void handleViewGroupHistory(const Group& group) {

    group.showGroupHistory();
}

void handleViewPersonalHistory(
    vector<Observer*>& users
) {

    string name;

    cout << "Whose personal chat history? ";

    cin >> name;

    Observer* u =
        getUserByName(name, users);

    User* user =
        dynamic_cast<User*>(u);

    if (user)
        user->showPersonalHistory();

    else
        cout << "Invalid name.\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Add / Remove Users
// ─────────────────────────────────────────────────────────────────────────────

void handleAddRemoveUser(
    Group& group,
    vector<Observer*>& users
) {

    string adminName;
    string action;
    string username;

    cout << "Who is performing this action? ";
    cin >> adminName;

    if (!group.isAdmin(adminName)) {

        cout << "You are not admin!\n";
        return;
    }

    cout << "Add or Remove? (add/remove): ";
    cin >> action;

    cout << "User name: ";
    cin >> username;

    if (action == "add") {

        Observer* existing =
            getUserByName(username, users);

        if (existing) {

            if (group.isMember(username)) {

                cout << username
                     << " is already in the group.\n";
            }
            else {

                group.add(existing);
            }
        }
        else {

            User* newUser =
                new User(username);

            users.push_back(newUser);

            group.add(newUser);
        }
    }

    else if (action == "remove") {

        group.remove(username);
    }

    else {

        cout << "Invalid option.\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Make Admin
// ─────────────────────────────────────────────────────────────────────────────

void handleMakeAdmin(Group& group) {

    string adminName;
    string newAdmin;

    cout << "Current admin name: ";
    cin >> adminName;

    cout << "New admin name: ";
    cin >> newAdmin;

    group.makeAdmin(adminName, newAdmin);
}

// ─────────────────────────────────────────────────────────────────────────────
// View Current Group Members
// ─────────────────────────────────────────────────────────────────────────────

void handleViewGroupMembers(
    const Group& group,
    vector<Observer*>& users
) {

    cout << "\n========== Current Group Members ==========\n";

    bool found = false;

    for (auto u : users) {

        if (group.isMember(u->getName())) {

            cout << "- " << u->getName();

            if (group.isAdmin(u->getName()))
                cout << " [ADMIN]";

            cout << endl;

            found = true;
        }
    }

    if (!found)
        cout << "No members in group.\n";

    cout << "===========================================\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────────────────────────────────────

int main() {

    printTextifyBanner();

    cout << "[Chat history is saved to .txt files "
         << "and reloaded on startup]\n\n";

    // Initial Users
    User* mosid   = new User("Mosid");
    User* saim    = new User("Saim");
    User* huzaifa = new User("Huzaifa");

    vector<Observer*> users =
    {
        mosid,
        saim,
        huzaifa
    };

    // Group
    Group group("TextifyGroup", "Mosid");

    for (auto u : users)
        group.add(u);

    int option = 0;

    while (option != 8) {

        displayMenu();

        cin >> option;

        cin.ignore();

        switch (option) {

            case 1:
                handleGroupMessage(group, users);
                break;

            case 2:
                handlePersonalMessage(users);
                break;

            case 3:
                handleViewGroupHistory(group);
                break;

            case 4:
                handleViewPersonalHistory(users);
                break;

            case 5:
                handleAddRemoveUser(group, users);
                break;

            case 6:
                handleMakeAdmin(group);
                break;

            case 7:
                handleViewGroupMembers(group, users);
                break;

            case 8:
                cout << "Exiting Textify. Goodbye!\n";
                break;

            default:
                cout << "Invalid option.\n";
        }
    }

    // Cleanup
    for (auto u : users)
        delete u;

    return 0;
}