#ifndef ad_sms_components_Memory_h
#define ad_sms_components_Memory_h


namespace ad {
namespace sms {
namespace components {

class Memory
{
public:
    /// \todo replace return value with value_8b
    unsigned char &operator[](std::size_t);
};

}}} // namespace ad::sms::components

#endif
