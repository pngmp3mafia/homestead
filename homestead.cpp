#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <fstream>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <thread>

// Custom Exception Classes
class ResourceException : public std::exception {
private:
    std::string message;
public:
    ResourceException(const std::string& msg) : message("Resource Error: " + msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class GameStateException : public std::exception {
private:
    std::string message;
public:
    GameStateException(const std::string& msg) : message("Game State Error: " + msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class ColonistException : public std::exception {
private:
    std::string message;
public:
    ColonistException(const std::string& msg) : message("Colonist Error: " + msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// Forward declarations
class GameEngine;
class GameState;
class Building;
class Colonist;
class Event;

// Resource Management Class with Operator Overloading
class Resource {
private:
    std::map<std::string, int> resources;

public:
    Resource() {
        resources["food"] = 100;
        resources["energy"] = 100;
        resources["materials"] = 50;
        resources["oxygen"] = 100;
    }

    // Operator overloading for resource management
    Resource operator+(const Resource& other) const {
        Resource result = *this;
        for(const auto& pair : other.resources) {
            result.resources[pair.first] += pair.second;
        }
        return result;
    }

    Resource operator-(const Resource& other) const {
        Resource result = *this;
        for(const auto& pair : other.resources) {
            result.resources[pair.first] -= pair.second;
            if(result.resources[pair.first] < 0) {
                throw ResourceException("Insufficient " + pair.first);
            }
        }
        return result;
    }

    Resource& operator+=(const Resource& other) {
        *this = *this + other;
        return *this;
    }

    Resource& operator-=(const Resource& other) {
        *this = *this - other;
        return *this;
    }

    int& operator[](const std::string& resourceType) {
        return resources[resourceType];
    }

    const int& operator[](const std::string& resourceType) const {
        auto it = resources.find(resourceType);
        if(it == resources.end()) {
            throw ResourceException("Unknown resource type: " + resourceType);
        }
        return it->second;
    }

    bool canAfford(const Resource& cost) const {
        for(const auto& pair : cost.resources) {
            auto it = resources.find(pair.first);
            if(it == resources.end() || it->second < pair.second) {
                return false;
            }
        }
        return true;
    }

    void display() const {
        std::cout << "Resources: ";
        for(const auto& pair : resources) {
            std::cout << pair.first << ":" << pair.second << " ";
        }
        std::cout << std::endl;
    }

    // File I/O support
    void saveToFile(std::ofstream& file) const {
        file << resources.size() << std::endl;
        for(const auto& pair : resources) {
            file << pair.first << " " << pair.second << std::endl;
        }
    }

    void loadFromFile(std::ifstream& file) {
        int count;
        file >> count;
        resources.clear();
        for(int i = 0; i < count; i++) {
            std::string type;
            int amount;
            file >> type >> amount;
            resources[type] = amount;
        }
    }
};

// Base Production Interface for Polymorphism
class Producible {
public:
    virtual ~Producible() = default;
    virtual Resource produce() = 0;
    virtual std::string getProductionInfo() const = 0;
};

// Base Building Class using Inheritance
class Building : public Producible {
protected:
    std::string name;
    Resource cost;
    Resource production;
    int level;
    bool operational;

public:
    Building(const std::string& buildingName) : 
        name(buildingName), level(1), operational(true) {}

    virtual ~Building() = default;

    // Pure virtual function for polymorphism
    virtual Resource produce() override = 0;
    virtual std::string getProductionInfo() const override = 0;

    // Common building operations
    virtual void upgrade() {
        level++;
        production["materials"] += 5;
    }

    virtual Resource getCost() const { return cost; }
    virtual std::string getName() const { return name; }
    virtual int getLevel() const { return level; }
    virtual bool isOperational() const { return operational; }
    virtual void setOperational(bool status) { operational = status; }

    // File I/O
    virtual void saveToFile(std::ofstream& file) const {
        file << name << " " << level << " " << operational << std::endl;
    }

    virtual void loadFromFile(std::ifstream& file) {
        file >> name >> level >> operational;
    }
};

// Derived Building Classes demonstrating Inheritance and Polymorphism
class SolarPanel : public Building {
public:
    SolarPanel() : Building("Solar Panel") {
        cost["materials"] = 20;
        production["energy"] = 15;
    }

    Resource produce() override {
        if(!operational) return Resource();
        Resource output;
        output["energy"] = production["energy"] * level;
        return output;
    }

    std::string getProductionInfo() const override {
        return "Solar Panel Level " + std::to_string(level) + 
               " produces " + std::to_string(production["energy"] * level) + " energy";
    }
};

class Greenhouse : public Building {
public:
    Greenhouse() : Building("Greenhouse") {
        cost["materials"] = 30;
        cost["energy"] = 10;
        production["food"] = 20;
    }

    Resource produce() override {
        if(!operational) return Resource();
        Resource output;
        output["food"] = production["food"] * level;
        return output;
    }

    std::string getProductionInfo() const override {
        return "Greenhouse Level " + std::to_string(level) + 
               " produces " + std::to_string(production["food"] * level) + " food";
    }
};

class OxygenGenerator : public Building {
public:
    OxygenGenerator() : Building("Oxygen Generator") {
        cost["materials"] = 25;
        cost["energy"] = 15;
        production["oxygen"] = 10;
    }

    Resource produce() override {
        if(!operational) return Resource();
        Resource output;
        output["oxygen"] = production["oxygen"] * level;
        return output;
    }

    std::string getProductionInfo() const override {
        return "Oxygen Generator Level " + std::to_string(level) + 
               " produces " + std::to_string(production["oxygen"] * level) + " oxygen";
    }
};

class MaterialFactory : public Building {
public:
    MaterialFactory() : Building("Material Factory") {
        cost["materials"] = 40;
        cost["energy"] = 20;
        production["materials"] = 8;
    }

    Resource produce() override {
        if(!operational) return Resource();
        Resource output;
        output["materials"] = production["materials"] * level;
        return output;
    }

    std::string getProductionInfo() const override {
        return "Material Factory Level " + std::to_string(level) + 
               " produces " + std::to_string(production["materials"] * level) + " materials";
    }
};

// Colonist Class with Skills and Specializations
class Colonist {
private:
    std::string name;
    std::string specialization;
    int experience;
    int health;
    bool assigned;

public:
    Colonist(const std::string& colonistName, const std::string& spec) : 
        name(colonistName), specialization(spec), experience(0), health(100), assigned(false) {}

    // Production routine based on specialization
    Resource work() {
        if(health < 50) {
            throw ColonistException(name + " is too sick to work");
        }

        Resource output;
        experience++;

        if(specialization == "Engineer") {
            output["materials"] = 5 + experience / 10;
        } else if(specialization == "Scientist") {
            output["energy"] = 3 + experience / 15;
            output["oxygen"] = 2 + experience / 20;
        } else if(specialization == "Farmer") {
            output["food"] = 8 + experience / 8;
        } else {
            output["materials"] = 2;
            output["food"] = 2;
        }

        return output;
    }

    void rest() {
        health = std::min(100, health + 10);
        assigned = false;
    }

    // Getters and setters
    std::string getName() const { return name; }
    std::string getSpecialization() const { return specialization; }
    int getExperience() const { return experience; }
    int getHealth() const { return health; }
    bool isAssigned() const { return assigned; }
    void setAssigned(bool status) { assigned = status; }
    void takeDamage(int damage) { 
        health = std::max(0, health - damage); 
        if(health == 0) {
            throw ColonistException(name + " has died");
        }
    }

    void displayInfo() const {
        std::cout << name << " (" << specialization << ") - Health: " << health 
                  << " Experience: " << experience << " Assigned: " << (assigned ? "Yes" : "No") << std::endl;
    }

    // File I/O
    void saveToFile(std::ofstream& file) const {
        file << name << " " << specialization << " " << experience << " " 
             << health << " " << assigned << std::endl;
    }

    void loadFromFile(std::ifstream& file) {
        file >> name >> specialization >> experience >> health >> assigned;
    }
};

// Event System for Random Events
class Event {
private:
    std::string name;
    std::string description;
    Resource resourceEffect;
    int probability;

public:
    Event(const std::string& eventName, const std::string& desc, int prob) : 
        name(eventName), description(desc), probability(prob) {}

    virtual ~Event() = default;

    // Virtual function for event execution
    virtual void execute(Resource& resources, std::vector<std::unique_ptr<Colonist>>& colonists) {
        std::cout << "Event: " << name << std::endl;
        std::cout << description << std::endl;
        
        try {
            resources += resourceEffect;
        } catch(const ResourceException& e) {
            std::cout << "Event partially failed: " << e.what() << std::endl;
        }
    }

    int getProbability() const { return probability; }
    std::string getName() const { return name; }

protected:
    void setResourceEffect(const std::string& resource, int amount) {
        resourceEffect[resource] = amount;
    }
};

// Specific Event Types
class SolarStorm : public Event {
public:
    SolarStorm() : Event("Solar Storm", "A solar storm damages energy systems!", 15) {
        setResourceEffect("energy", -30);
    }

    void execute(Resource& resources, std::vector<std::unique_ptr<Colonist>>& colonists) override {
        Event::execute(resources, colonists);
        // Additional effects specific to solar storm
        for(auto& colonist : colonists) {
            if(colonist->getSpecialization() == "Engineer") {
                std::cout << colonist->getName() << " quickly repairs some damage!" << std::endl;
                resources["energy"] += 10;
                break;
            }
        }
    }
};

class TradeShip : public Event {
public:
    TradeShip() : Event("Trade Ship Arrival", "A trade ship offers resources!", 25) {
        setResourceEffect("materials", 20);
        setResourceEffect("food", 15);
    }
};

class MeteorShower : public Event {
public:
    MeteorShower() : Event("Meteor Shower", "Meteors provide rare materials but damage buildings!", 10) {
        setResourceEffect("materials", 30);
        setResourceEffect("oxygen", -10);
    }
};

// Game State Management
enum class GamePhase {
    SETUP,
    PRODUCTION,
    EVENT,
    MANAGEMENT,
    END
};

class GameState {
private:
    GamePhase currentPhase;
    int turn;
    int colonistCount;
    bool gameRunning;

public:
    GameState() : currentPhase(GamePhase::SETUP), turn(1), colonistCount(0), gameRunning(true) {}

    void nextPhase() {
        switch(currentPhase) {
            case GamePhase::SETUP:
                currentPhase = GamePhase::PRODUCTION;
                break;
            case GamePhase::PRODUCTION:
                currentPhase = GamePhase::EVENT;
                break;
            case GamePhase::EVENT:
                currentPhase = GamePhase::MANAGEMENT;
                break;
            case GamePhase::MANAGEMENT:
                currentPhase = GamePhase::PRODUCTION;
                turn++;
                break;
            case GamePhase::END:
                gameRunning = false;
                break;
        }
    }

    void endGame() {
        currentPhase = GamePhase::END;
        gameRunning = false;
    }

    // State queries
    GamePhase getCurrentPhase() const { return currentPhase; }
    int getTurn() const { return turn; }
    bool isGameRunning() const { return gameRunning; }
    int getColonistCount() const { return colonistCount; }
    void setColonistCount(int count) { colonistCount = count; }

    std::string getPhaseString() const {
        switch(currentPhase) {
            case GamePhase::SETUP: return "Setup";
            case GamePhase::PRODUCTION: return "Production";
            case GamePhase::EVENT: return "Event";
            case GamePhase::MANAGEMENT: return "Management";
            case GamePhase::END: return "Game Over";
            default: return "Unknown";
        }
    }

    // File I/O
    void saveToFile(std::ofstream& file) const {
        file << static_cast<int>(currentPhase) << " " << turn << " " 
             << colonistCount << " " << gameRunning << std::endl;
    }

    void loadFromFile(std::ifstream& file) {
        int phase;
        file >> phase >> turn >> colonistCount >> gameRunning;
        currentPhase = static_cast<GamePhase>(phase);
    }
};

// Main Game Engine Class
class GameEngine {
private:
    GameState gameState;
    Resource colonyResources;
    std::vector<std::unique_ptr<Building>> buildings;
    std::vector<std::unique_ptr<Colonist>> colonists;
    std::vector<std::unique_ptr<Event>> events;
    std::mt19937 randomGenerator;

    // Configuration data
    std::map<std::string, std::string> config;

public:
    GameEngine() : randomGenerator(std::chrono::steady_clock::now().time_since_epoch().count()) {
        initializeGame();
    }

    void initializeGame() {
        loadConfiguration();
        setupEvents();
        
        // Create initial colonists
        colonists.push_back(std::make_unique<Colonist>("Alex Chen", "Engineer"));
        colonists.push_back(std::make_unique<Colonist>("Maria Santos", "Scientist"));
        colonists.push_back(std::make_unique<Colonist>("James Wilson", "Farmer"));
        
        gameState.setColonistCount(colonists.size());

        // Initial buildings
        buildings.push_back(std::make_unique<SolarPanel>());
        buildings.push_back(std::make_unique<Greenhouse>());

        std::cout << "Stellar Homestead Colony Established!" << std::endl;
        std::cout << "Starting resources and colonists initialized." << std::endl;
    }

    void setupEvents() {
        events.push_back(std::make_unique<SolarStorm>());
        events.push_back(std::make_unique<TradeShip>());
        events.push_back(std::make_unique<MeteorShower>());
    }

    void runGameLoop() {
        while(gameState.isGameRunning()) {
            displayGameStatus();
            
            try {
                switch(gameState.getCurrentPhase()) {
                    case GamePhase::SETUP:
                        handleSetupPhase();
                        break;
                    case GamePhase::PRODUCTION:
                        handleProductionPhase();
                        break;
                    case GamePhase::EVENT:
                        handleEventPhase();
                        break;
                    case GamePhase::MANAGEMENT:
                        handleManagementPhase();
                        break;
                    case GamePhase::END:
                        handleEndGame();
                        break;
                }
            } catch(const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
                handleError();
            }

            gameState.nextPhase();
            
            // Check win/lose conditions
            checkGameConditions();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    void handleSetupPhase() {
        std::cout << "\n=== Setup Phase ===" << std::endl;
        std::cout << "Colony initialization complete. Press Enter to continue...";
        std::cin.get();
    }

    void handleProductionPhase() {
        std::cout << "\n=== Production Phase ===" << std::endl;
        
        Resource totalProduction;
        
        // Building production
        for(auto& building : buildings) {
            if(building->isOperational()) {
                Resource buildingOutput = building->produce();
                totalProduction += buildingOutput;
                std::cout << building->getProductionInfo() << std::endl;
            }
        }

        // Colonist work
        for(auto& colonist : colonists) {
            if(!colonist->isAssigned() && colonist->getHealth() > 50) {
                Resource colonistOutput = colonist->work();
                totalProduction += colonistOutput;
                std::cout << colonist->getName() << " worked and produced resources." << std::endl;
            }
        }

        // Apply production to colony resources
        colonyResources += totalProduction;
        
        // Resource consumption per turn
        Resource consumption;
        consumption["food"] = colonists.size() * 3;
        consumption["oxygen"] = colonists.size() * 2;
        consumption["energy"] = buildings.size() * 2;
        
        colonyResources -= consumption;
        
        std::cout << "Total production applied. Resource consumption deducted." << std::endl;
    }

    void handleEventPhase() {
        std::cout << "\n=== Event Phase ===" << std::endl;
        
        std::uniform_int_distribution<> eventChance(1, 100);
        int roll = eventChance(randomGenerator);
        
        bool eventTriggered = false;
        for(auto& event : events) {
            if(roll <= event->getProbability()) {
                event->execute(colonyResources, colonists);
                eventTriggered = true;
                break;
            }
        }
        
        if(!eventTriggered) {
            std::cout << "A peaceful turn. No events occurred." << std::endl;
        }
    }

    void handleManagementPhase() {
        std::cout << "\n=== Management Phase ===" << std::endl;
        std::cout << "1. Build Structure" << std::endl;
        std::cout << "2. Assign Colonists" << std::endl;
        std::cout << "3. Rest Colonists" << std::endl;
        std::cout << "4. Save Game" << std::endl;
        std::cout << "5. Continue to next turn" << std::endl;
        std::cout << "Choose action: ";
        
        int choice;
        std::cin >> choice;
        
        switch(choice) {
            case 1:
                buildStructure();
                break;
            case 2:
                assignColonists();
                break;
            case 3:
                restColonists();
                break;
            case 4:
                saveGame();
                break;
            case 5:
            default:
                std::cout << "Continuing to next turn..." << std::endl;
                break;
        }
    }

    void buildStructure() {
        std::cout << "Available structures:" << std::endl;
        std::cout << "1. Solar Panel (Materials: 20)" << std::endl;
        std::cout << "2. Greenhouse (Materials: 30, Energy: 10)" << std::endl;
        std::cout << "3. Oxygen Generator (Materials: 25, Energy: 15)" << std::endl;
        std::cout << "4. Material Factory (Materials: 40, Energy: 20)" << std::endl;
        
        int choice;
        std::cin >> choice;
        
        std::unique_ptr<Building> newBuilding;
        
        switch(choice) {
            case 1:
                newBuilding = std::make_unique<SolarPanel>();
                break;
            case 2:
                newBuilding = std::make_unique<Greenhouse>();
                break;
            case 3:
                newBuilding = std::make_unique<OxygenGenerator>();
                break;
            case 4:
                newBuilding = std::make_unique<MaterialFactory>();
                break;
            default:
                std::cout << "Invalid choice." << std::endl;
                return;
        }
        
        Resource cost = newBuilding->getCost();
        if(colonyResources.canAfford(cost)) {
            colonyResources -= cost;
            std::cout << "Built " << newBuilding->getName() << "!" << std::endl;
            buildings.push_back(std::move(newBuilding));
        } else {
            std::cout << "Insufficient resources to build " << newBuilding->getName() << std::endl;
        }
    }

    void assignColonists() {
        std::cout << "Available colonists:" << std::endl;
        for(size_t i = 0; i < colonists.size(); i++) {
            std::cout << i + 1 << ". ";
            colonists[i]->displayInfo();
        }
        
        std::cout << "Select colonist to assign (0 to cancel): ";
        size_t choice;
        std::cin >> choice;
        
        if(choice > 0 && choice <= colonists.size()) {
            colonists[choice - 1]->setAssigned(true);
            std::cout << colonists[choice - 1]->getName() << " has been assigned to work." << std::endl;
        }
    }

    void restColonists() {
        for(auto& colonist : colonists) {
            colonist->rest();
        }
        std::cout << "All colonists have rested and recovered health." << std::endl;
    }

    void displayGameStatus() {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "STELLAR HOMESTEAD - Turn " << gameState.getTurn() << std::endl;
        std::cout << "Phase: " << gameState.getPhaseString() << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        colonyResources.display();
        
        std::cout << "Buildings (" << buildings.size() << "):" << std::endl;
        for(const auto& building : buildings) {
            std::cout << "  " << building->getName() << " Level " << building->getLevel()
                      << " (" << (building->isOperational() ? "Operational" : "Offline") << ")" << std::endl;
        }
        
        std::cout << "Colonists (" << colonists.size() << "):" << std::endl;
        for(const auto& colonist : colonists) {
            std::cout << "  ";
            colonist->displayInfo();
        }
    }

    void checkGameConditions() {
        // Win condition: 10 turns survived with healthy colony
        if(gameState.getTurn() >= 10 && colonists.size() >= 3) {
            std::cout << "\nCongratulations! Your colony has thrived for 10 turns!" << std::endl;
            gameState.endGame();
            return;
        }
        
        // Lose conditions
        if(colonyResources["food"] <= 0 || colonyResources["oxygen"] <= 0) {
            std::cout << "\nGame Over! Your colony has run out of essential resources." << std::endl;
            gameState.endGame();
            return;
        }
        
        if(colonists.empty()) {
            std::cout << "\nGame Over! All colonists have perished." << std::endl;
            gameState.endGame();
            return;
        }
    }

    void handleError() {
        std::cout << "An error occurred. Attempting to continue..." << std::endl;
        // Error recovery logic here
    }

    void handleEndGame() {
        std::cout << "\nGame ended after " << gameState.getTurn() << " turns." << std::endl;
        std::cout << "Final colony status:" << std::endl;
        colonyResources.display();
        std::cout << "Thank you for playing Stellar Homestead!" << std::endl;
    }

    // File I/O for game save/load
    void saveGame() {
        try {
            std::ofstream file("stellar_homestead_save.txt");
            
            // Save game state
            gameState.saveToFile(file);
            
            // Save resources
            colonyResources.saveToFile(file);
            
            // Save buildings
            file << buildings.size() << std::endl;
            for(const auto& building : buildings) {
                building->saveToFile(file);
            }
            
            // Save colonists
            file << colonists.size() << std::endl;
            for(const auto& colonist : colonists) {
                colonist->saveToFile(file);
            }
            
            file.close();
            std::cout << "Game saved successfully!" << std::endl;
            
        } catch(const std::exception& e) {
            std::cout << "Failed to save game: " << e.what() << std::endl;
        }
    }

    void loadGame() {
        try {
            std::ifstream file("stellar_homestead_save.txt");
            
            // Load game state
            gameState.loadFromFile(file);
            
            // Load resources
            colonyResources.loadFromFile(file);
            
            // Load buildings
            int buildingCount;
            file >> buildingCount;
            buildings.clear();
            // Note: In a full implementation, you'd need a factory pattern
            // to recreate the correct building types from saved data
            
            // Load colonists
            int colonistCount;
            file >> colonistCount;
            colonists.clear();
            // Similar factory pattern needed for colonists
            
            file.close();
            std::cout << "Game loaded successfully!" << std::endl;
            
        } catch(const std::exception& e) {
            std::cout << "Failed to load game: " << e.what() << std::endl;
        }
    }

    void loadConfiguration() {
        try {
            std::ifstream configFile("config.txt");
            std::string key, value;
            
            while(configFile >> key >> value) {
                config[key] = value;
            }
            
            configFile.close();
            
            // Apply configuration settings
            if(config.find("difficulty") != config.end()) {
                std::cout << "Difficulty set to: " << config["difficulty"] << std::endl;
            }
            
        } catch(const std::exception& e) {
            std::cout << "Using default configuration." << std::endl;
            // Set default config values
            config["difficulty"] = "normal";
            config["auto_save"] = "true";
        }
    }
};

// Main function
int main() {
    try {
        std::cout << "Welcome to Stellar Homestead!" << std::endl;
        std::cout << "A space colony management simulation." << std::endl;
        std::cout << "Manage resources, build structures, and keep your colonists alive!" << std::endl;
        
        GameEngine game;
        
        std::cout << "\nPress Enter to start the game...";
        std::cin.get();
        
        game.runGameLoop();
        
    } catch(const std::exception& e) {
        std::cout << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}