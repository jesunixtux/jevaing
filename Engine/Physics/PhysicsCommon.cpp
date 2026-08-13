#include <Jevaing/Physics.h>

#include <algorithm>
#include <cctype>

namespace Jevaing
{
    const char* Physics2DBackendToString(Physics2DBackend backend)
    {
        switch (backend)
        {
            case Physics2DBackend::None:
                return "None";
            case Physics2DBackend::Box2D:
                return "Box2D";
        }

        return "Unknown";
    }

    const char* Physics3DBackendToString(Physics3DBackend backend)
    {
        switch (backend)
        {
            case Physics3DBackend::None:
                return "None";
            case Physics3DBackend::Jolt:
                return "Jolt";
        }

        return "Unknown";
    }

    const char* BodyTypeToString(BodyType type)
    {
        switch (type)
        {
            case BodyType::Static:
                return "static";
            case BodyType::Kinematic:
                return "kinematic";
            case BodyType::Dynamic:
                return "dynamic";
        }

        return "unknown";
    }

    bool BodyTypeFromString(const std::string& value, BodyType& type)
    {
        std::string normalized = value;
        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            }
        );

        if (normalized == "static")
        {
            type = BodyType::Static;
            return true;
        }

        if (normalized == "kinematic")
        {
            type = BodyType::Kinematic;
            return true;
        }

        if (normalized == "dynamic")
        {
            type = BodyType::Dynamic;
            return true;
        }

        return false;
    }
}
