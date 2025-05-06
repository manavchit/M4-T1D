#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>
#include <numeric>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <random>
#include <functional>
#include <variant>
#include <iomanip>
#include <optional>

using namespace std;
using namespace std::chrono;

// Color coding macros
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

// --------------------------
// Core Domain Models (OOP)
// --------------------------

enum class GradeLevel { FRESHMAN, SOPHOMORE, JUNIOR, SENIOR };

GradeLevel stringToGradeLevel(const string& str) {
    if (str == "FRESHMAN") return GradeLevel::FRESHMAN;
    if (str == "SOPHOMORE") return GradeLevel::SOPHOMORE;
    if (str == "JUNIOR") return GradeLevel::JUNIOR;
    if (str == "SENIOR") return GradeLevel::SENIOR;
    throw runtime_error("Invalid grade level: " + str);
}

string gradeLevelToString(GradeLevel level) {
    switch(level) {
        case GradeLevel::FRESHMAN: return "FRESHMAN";
        case GradeLevel::SOPHOMORE: return "SOPHOMORE";
        case GradeLevel::JUNIOR: return "JUNIOR";
        case GradeLevel::SENIOR: return "SENIOR";
        default: return "UNKNOWN";
    }
}

struct Address {
    string street;
    string city;
    string state;
    string zip_code;
};

class Person {
protected:
    string id;
    string name;
    string email;
    Address address;
    system_clock::time_point created_at;

public:
    Person(string id, string name, string email, Address address)
        : id(id), name(name), email(email), address(address), created_at(system_clock::now()) {}

    virtual ~Person() = default;

    virtual string role() const = 0;

    virtual map<string, string> info() const {
        return {
            {"id", id},
            {"name", name},
            {"email", email},
            {"street", address.street},
            {"city", address.city},
            {"state", address.state},
            {"zip_code", address.zip_code},
            {"created_at", to_string(created_at.time_since_epoch().count())}
        };
    }

    const string& getId() const { return id; }
    const string& getName() const { return name; }
    const string& getEmail() const { return email; }
    const Address& getAddress() const { return address; }
};

class Observer {
public:
    virtual ~Observer() = default;
    virtual void update(const string& student_id, const string& course_id, optional<float> old_wam, float new_wam) = 0;
};

class Observable {
    vector<Observer*> observers;
    mutex mtx;

public:
    void addObserver(Observer* observer) {
        lock_guard<mutex> lock(mtx);
        observers.push_back(observer);
    }

    void notify(const string& student_id, const string& course_id, optional<float> old_wam, float new_wam) {
        lock_guard<mutex> lock(mtx);
        for (auto observer : observers) {
            observer->update(student_id, course_id, old_wam, new_wam);
        }
    }
};

class Student : public Person {
    GradeLevel grade_level;
    map<string, optional<float>> courses; // course_id -> WAM score
    Observable observable;

public:
    Student(string id, string name, string email, Address address, GradeLevel grade_level)
        : Person(id, name, email, address), grade_level(grade_level) {}

    string role() const override { return "Student"; }

    void enroll(const string& course_id) {
        if (courses.find(course_id) == courses.end()) {
            courses[course_id] = nullopt;
            observable.notify(id, course_id, nullopt, 0.0f);
        }
    }

    void updateWAM(const string& course_id, float wam) {
        if (wam < 0 || wam > 100) {
            cerr << RED << "Invalid WAM score. Must be between 0 and 100." << RESET << endl;
            return;
        }
        
        if (courses.find(course_id) != courses.end()) {
            auto old_wam = courses[course_id];
            courses[course_id] = wam;
            observable.notify(id, course_id, old_wam, wam);
        }
    }

    float overallWAM() const {
        vector<float> wams;
        for (const auto& course : courses) {
            if (course.second.has_value()) {
                wams.push_back(course.second.value());
            }
        }
        
        if (wams.empty()) return 0.0f;
        return accumulate(wams.begin(), wams.end(), 0.0f) / wams.size();
    }

    void addObserver(Observer* observer) {
        observable.addObserver(observer);
    }

    const map<string, optional<float>>& getCourses() const { return courses; }
    GradeLevel getGradeLevel() const { return grade_level; }
};

class Teacher : public Person {
    string department;
    string specialization;
    set<string> assigned_courses;

public:
    Teacher(string id, string name, string email, Address address, string department, string specialization)
        : Person(id, name, email, address), department(department), specialization(specialization) {}

    string role() const override { return "Teacher"; }

    void assignCourse(const string& course_id) {
        assigned_courses.insert(course_id);
    }

    int courseLoad() const {
        return assigned_courses.size();
    }

    const string& getDepartment() const { return department; }
    const string& getSpecialization() const { return specialization; }
    const set<string>& getAssignedCourses() const { return assigned_courses; }
};

class Course {
    string id;
    string name;
    int credits;
    int capacity;
    set<string> enrolled_students;
    set<string> prerequisites;

public:
    Course(string id, string name, int credits, int capacity = 30)
        : id(id), name(name), credits(credits), capacity(capacity) {}

    void addPrerequisite(const string& course_id) {
        prerequisites.insert(course_id);
    }

    bool enrollStudent(const string& student_id) {
        if (enrolled_students.size() < capacity) {
            enrolled_students.insert(student_id);
            return true;
        }
        return false;
    }

    int availableSeats() const {
        return capacity - enrolled_students.size();
    }

    const string& getId() const { return id; }
    const string& getName() const { return name; }
    int getCredits() const { return credits; }
    int getCapacity() const { return capacity; }
    int getEnrolledCount() const { return enrolled_students.size(); }
    const set<string>& getPrerequisites() const { return prerequisites; }
    const set<string>& getEnrolledStudents() const { return enrolled_students; }
};

// --------------------------
// School Management System
// --------------------------

class School {
    string name;
    vector<shared_ptr<Student>> students;
    vector<shared_ptr<Teacher>> teachers;
    vector<shared_ptr<Course>> courses;
    mutex mtx;

public:
    School(string name) : name(name) {}

    void addStudent(shared_ptr<Student> student) {
        lock_guard<mutex> lock(mtx);
        students.push_back(student);
    }

    void addTeacher(shared_ptr<Teacher> teacher) {
        lock_guard<mutex> lock(mtx);
        teachers.push_back(teacher);
    }

    void addCourse(shared_ptr<Course> course) {
        lock_guard<mutex> lock(mtx);
        courses.push_back(course);
    }

    bool enrollStudentInCourse(const string& student_id, const string& course_id) {
        lock_guard<mutex> lock(mtx);
        
        auto student_it = find_if(students.begin(), students.end(), 
            [&student_id](const shared_ptr<Student>& s) { return s->getId() == student_id; });
        
        auto course_it = find_if(courses.begin(), courses.end(), 
            [&course_id](const shared_ptr<Course>& c) { return c->getId() == course_id; });

        if (student_it == students.end() || course_it == courses.end()) {
            cerr << RED << "Student or course not found!" << RESET << endl;
            return false;
        }

        if ((*course_it)->enrollStudent(student_id)) {
            (*student_it)->enroll(course_id);
            return true;
        }
        
        cerr << RED << "Course " << course_id << " is full!" << RESET << endl;
        return false;
    }

    map<string, int> getDepartmentStats() const {
        map<string, int> stats;
        for (const auto& teacher : teachers) {
            stats[teacher->getDepartment()]++;
        }
        return stats;
    }

    vector<pair<string, float>> getTopPerformers(int n) const {
        vector<pair<string, float>> performers;
        for (const auto& student : students) {
            performers.emplace_back(student->getName(), student->overallWAM());
        }

        sort(performers.begin(), performers.end(), 
            [](const pair<string, float>& a, const pair<string, float>& b) {
                return a.second > b.second;
            });

        if (n > performers.size()) n = performers.size();
        return vector<pair<string, float>>(performers.begin(), performers.begin() + n);
    }

    vector<string> generateAllStudentReports() const {
        vector<future<string>> futures;
        vector<string> reports;

        for (const auto& student : students) {
            futures.push_back(async(launch::async, [&student]() {
                stringstream ss;
                ss << "Student Report for " << student->getName() << " (" << student->getId() << ")\n";
                ss << "Grade Level: " << gradeLevelToString(student->getGradeLevel()) << "\n";
                ss << "Overall WAM: " << fixed << setprecision(1) << student->overallWAM() << "\n";
                ss << "Courses:\n";
                
                auto courses = student->getCourses();
                for (const auto& [course_id, wam] : courses) {
                    ss << " - " << course_id << ": ";
                    if (wam.has_value()) {
                        ss << wam.value();
                    } else {
                        ss << "No grade yet";
                    }
                    ss << "\n";
                }
                return ss.str();
            }));
        }

        for (auto& f : futures) {
            reports.push_back(f.get());
        }

        return reports;
    }

    void simulateWAMUpdates() {
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> dis(50.0, 95.0);

        cout << BOLD << MAGENTA << "\nSimulating WAM updates..." << RESET << endl;
        
        for (const auto& student : students) {
            for (const auto& [course_id, _] : student->getCourses()) {
                float new_wam = dis(gen);
                student->updateWAM(course_id, new_wam);
                
                cout << CYAN << "Updated " << student->getName() << "'s " << course_id 
                     << " to " << fixed << setprecision(1) << new_wam << RESET << endl;
                this_thread::sleep_for(milliseconds(100));
            }
        }
    }

    const vector<shared_ptr<Student>>& getStudents() const { return students; }
    const vector<shared_ptr<Teacher>>& getTeachers() const { return teachers; }
    const vector<shared_ptr<Course>>& getCourses() const { return courses; }
};

// --------------------------
// Visitor Pattern
// --------------------------

class Visitor {
public:
    virtual ~Visitor() = default;
    virtual string visit(const Student& student) = 0;
    virtual string visit(const Teacher& teacher) = 0;
    virtual string visit(const Course& course) = 0;
};

class DisplayVisitor : public Visitor {
public:
    string visit(const Student& student) override {
        stringstream ss;
        ss << BOLD << BLUE << "STUDENT" << RESET << "\n";
        ss << "Name: " << student.getName() << "\n";
        ss << "ID: " << student.getId() << "\n";
        ss << "Grade Level: " << gradeLevelToString(student.getGradeLevel()) << "\n";
        ss << "Email: " << student.getEmail() << "\n";
        ss << "Address: " << student.getAddress().street << ", " 
           << student.getAddress().city << ", " << student.getAddress().state << "\n";
        ss << "WAM: " << fixed << setprecision(1) << student.overallWAM() << "\n";
        return ss.str();
    }

    string visit(const Teacher& teacher) override {
        stringstream ss;
        ss << BOLD << GREEN << "TEACHER" << RESET << "\n";
        ss << "Name: " << teacher.getName() << "\n";
        ss << "ID: " << teacher.getId() << "\n";
        ss << "Department: " << teacher.getDepartment() << "\n";
        ss << "Specialization: " << teacher.getSpecialization() << "\n";
        ss << "Email: " << teacher.getEmail() << "\n";
        ss << "Address: " << teacher.getAddress().street << ", " 
           << teacher.getAddress().city << ", " << teacher.getAddress().state << "\n";
        return ss.str();
    }

    string visit(const Course& course) override {
        stringstream ss;
        ss << BOLD << YELLOW << "COURSE" << RESET << "\n";
        ss << "Name: " << course.getName() << "\n";
        ss << "ID: " << course.getId() << "\n";
        ss << "Credits: " << course.getCredits() << "\n";
        ss << "Enrolled: " << course.getEnrolledCount() << "/" << course.getCapacity() << "\n";
        return ss.str();
    }
};

// --------------------------
// File Reading Functions
// --------------------------

vector<shared_ptr<Student>> readStudentsFromFile(const string& filename) {
    vector<shared_ptr<Student>> students;
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        vector<string> tokens;

        while (getline(ss, token, ',')) {
            // Trim whitespace from token
            token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
            token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);
            tokens.push_back(token);
        }

        if (tokens.size() != 8) {
            cerr << RED << "Invalid student record: " << line << RESET << endl;
            continue;
        }

        Address addr{tokens[3], tokens[4], tokens[5], tokens[6]};
        students.push_back(make_shared<Student>(
            tokens[0], tokens[1], tokens[2], addr, stringToGradeLevel(tokens[7])
        ));
    }

    return students;
}

vector<shared_ptr<Teacher>> readTeachersFromFile(const string& filename) {
    vector<shared_ptr<Teacher>> teachers;
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        vector<string> tokens;

        while (getline(ss, token, ',')) {
            // Trim whitespace from token
            token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
            token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);
            tokens.push_back(token);
        }

        if (tokens.size() != 9) {
            cerr << RED << "Invalid teacher record: " << line << RESET << endl;
            continue;
        }

        Address addr{tokens[3], tokens[4], tokens[5], tokens[6]};
        teachers.push_back(make_shared<Teacher>(
            tokens[0], tokens[1], tokens[2], addr, tokens[7], tokens[8]
        ));
    }

    return teachers;
}

// --------------------------
// Main Function
// --------------------------

int main() {
    try {
        // Initialize school
        School school("Chitkara University");

        // Read data from files
        auto students = readStudentsFromFile("students.txt");
        auto teachers = readTeachersFromFile("teachers.txt");

        // Add students and teachers to school
        for (const auto& student : students) {
            school.addStudent(student);
        }
        for (const auto& teacher : teachers) {
            school.addTeacher(teacher);
        }

        // Add courses
        vector<shared_ptr<Course>> course_list = {
            make_shared<Course>("CS101", "Programming Paradigms", 4),
            make_shared<Course>("CS201", "Network and Communication", 4),
            make_shared<Course>("CS301", "Backend Development", 4),
            make_shared<Course>("PD101", "Professional Development", 3)
        };
        for (const auto& course : course_list) {
            school.addCourse(course);
        }

        // Assign courses to teachers
        for (const auto& teacher : teachers) {
            if (teacher->getSpecialization() == "Programming Paradigms") {
                teacher->assignCourse("CS101");
            } else if (teacher->getSpecialization() == "Network and Communication") {
                teacher->assignCourse("CS201");
            } else if (teacher->getSpecialization() == "Backend Development") {
                teacher->assignCourse("CS301");
            } else if (teacher->getSpecialization() == "Career Skills") {
                teacher->assignCourse("PD101");
            }
        }

        // Enroll students in courses
        vector<pair<string, string>> enrollments = {
            {"S001", "CS101"}, {"S001", "CS201"}, {"S001", "PD101"},
            {"S002", "CS101"}, {"S002", "CS301"}, {"S002", "PD101"},
            {"S003", "CS201"}, {"S003", "CS301"}, {"S003", "PD101"},
            {"S004", "CS101"}, {"S004", "PD101"},
            {"S005", "CS101"}, {"S005", "CS201"},
            {"S006", "CS101"}, {"S006", "CS301"},
            {"S007", "CS101"},
            {"S008", "CS101"}, {"S008", "PD101"},
            {"S009", "CS201"}, {"S009", "PD101"},
            {"S010", "CS101"}, {"S010", "CS201"}, {"S010", "CS301"}
        };
        
        for (const auto& [student_id, course_id] : enrollments) {
            school.enrollStudentInCourse(student_id, course_id);
        }

        // Demonstrate functional programming
        cout << BOLD << BLUE << "\nDepartment Statistics:" << RESET << endl;
        auto dept_stats = school.getDepartmentStats();
        for (const auto& [dept, count] : dept_stats) {
            cout << CYAN << dept << RESET << ": " << count << " teachers" << endl;
        }

        // Demonstrate concurrent report generation
        cout << BOLD << BLUE << "\nGenerating reports concurrently..." << RESET << endl;
        auto reports = school.generateAllStudentReports();
        cout << GREEN << "Generated " << reports.size() << " student reports" << RESET << endl;

        // Demonstrate reactive programming with WAM updates
        school.simulateWAMUpdates();

        // Show updated top performers with color coding
        cout << BOLD << BLUE << "\nTop 3 Performers:" << RESET << endl;
        auto top_performers = school.getTopPerformers(3);
        for (const auto& [name, wam] : top_performers) {
            string color = GREEN;
            if (wam < 70) color = YELLOW;
            if (wam < 60) color = RED;
            
            cout << BOLD << name << RESET << ": " << color << fixed << setprecision(1) << wam << RESET << endl;
        }

        // Demonstrate visitor pattern with color coding (DISPLAY ALL)
        DisplayVisitor visitor;
        cout << BOLD << BLUE << "\nDisplaying ALL information with visitor pattern:" << RESET << endl;

        // Display all students
        cout << BOLD << MAGENTA << "\n=== ALL STUDENTS ===" << RESET << endl;
        for (const auto& student : school.getStudents()) {
            cout << visitor.visit(*student) << endl;
        }

        // Display all teachers
        cout << BOLD << MAGENTA << "\n=== ALL TEACHERS ===" << RESET << endl;
        for (const auto& teacher : school.getTeachers()) {
            cout << visitor.visit(*teacher) << endl;
        }

        // Display all courses
        cout << BOLD << MAGENTA << "\n=== ALL COURSES ===" << RESET << endl;
        for (const auto& course : school.getCourses()) {
            cout << visitor.visit(*course) << endl;
        }

    } catch (const exception& e) {
        cerr << RED << "Error: " << e.what() << RESET << endl;
        return 1;
    }

    return 0;
}
