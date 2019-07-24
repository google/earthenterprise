#ifndef _MEM_USAGE_YAML_CPP_WRAPPER_HPP
#define _MEM_USAGE_YAML_CPP_WRAPPER_HPP

#include <type_traits>

#include <yaml-cpp/yaml.h>


namespace opengee {

namespace mem_usage {

// A wrapper around the YAML::Emitter class that provides a consrtuctor taking
// an `std::ostream` argument whether or not the underlying `YAML::Emitter`
// class provides such a constructor.
//     That constructor is usend to be able to write YAML to an output stream
// immediately (live updates).  The old `yaml-cpp` API (`yaml-cpp` before
// version 0.5) doesn't provide such a constructor, but stores the whole
// document in memory.
template<
    // CppYamlHasNewApi will be set to std::true_type if a constructor with
    // signature YAML::Emitter(std::ostream&) exists:
    typename CppYamlHasNewApi =
        std::is_constructible<YAML::Emitter, std::ostream&>::type
>
class YamlEmitterWrapper : public YAML::Emitter
{
    YamlEmitterWrapper(std::ostream &output_stream)
    {}
};

// Class specialization for CppYamlHasNewApi = std::true_type, i.e. when the
// YAML::Emitter(std::ostream&) constructor exists:
template<>
class YamlEmitterWrapper<std::true_type> : public YAML::Emitter
{
    public:

    template <typename ... Args>
    YamlEmitterWrapper(Args&&... args)
    : YAML::Emitter{args...}
    {
        // std::cerr << "New API constructor." << std::endl;
    }
};

// Class specialization for CppYamlHasNewApi = std::false_type, i.e. when the
// YAML::Emitter(std::ostream&) constructor doesn't exists:
template<>
class YamlEmitterWrapper<std::false_type> : public YAML::Emitter
{
    private:

    std::ostream &output_stream;
    unsigned long output_offset;

    public:

    YamlEmitterWrapper(std::ostream &output_stream)
    : output_stream(output_stream),
        output_offset(0)
    {
        // std::cerr << "Old API constructor." << std::endl;
    }

    template<typename OutputType>
    friend inline YamlEmitterWrapper<std::false_type>& operator << (
        YamlEmitterWrapper<std::false_type>& emitter, OutputType v);
};

template <
    typename OutputType
>
inline YamlEmitterWrapper<std::false_type>& operator << (
    YamlEmitterWrapper<std::false_type>& emitter, OutputType v
) {
    operator<<(static_cast<YAML::Emitter&>(emitter), v);
    
    emitter.output_stream << (emitter.c_str() + emitter.output_offset);
    emitter.output_offset = emitter.size();

    return emitter;
}


// Be able to declare variables without the `<>` empty template syntax:
using YamlEmitter = YamlEmitterWrapper<>;

}

}

#endif // _MEM_USAGE_YAML_CPP_WRAPPER_HPP
