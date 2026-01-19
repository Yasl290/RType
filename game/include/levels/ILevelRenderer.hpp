#pragma once
#include <vector>
#include <string>

class ILevelRenderer {
public:
    virtual ~ILevelRenderer() = default;
    

    virtual const std::vector<std::string>& getEnemyBasicFrames() const = 0;
    virtual const std::vector<std::string>& getBossFrames() const = 0;
    virtual const std::vector<std::string>& getBossShotFrames() const = 0;
    virtual const std::string& getBackgroundPath() const = 0;
    virtual const std::string& getMusicPath() const = 0;
    virtual const char* getName() const = 0;
};