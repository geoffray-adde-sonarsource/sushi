#include "gtest/gtest.h"

#define private public

#include "library/plugin_manager.h"

using namespace sushi;

class TestPlugin : public StompBox
{
public:
    TestPlugin() {}

    StompBoxStatus init(const StompBoxConfig& /*configuration*/) override {return StompBoxStatus::OK;}

    std::string unique_id() const override {return "test_plugin";}

    void process_event(BaseEvent* /*event*/) override {}

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override
    {
        *out_buffer = *in_buffer;
    }
};


class StompBoxManagerTest : public ::testing::Test
{
protected:
    StompBoxManagerTest()
    {
    }
    void SetUp()
    {
        _test_plugin = new TestPlugin;
        _module_under_test = new StompBoxManager(_test_plugin);
    }

    void TearDown()
    {
        // _test_plugin is managed by the StompBoxManager and should not be deleted
        delete(_module_under_test);
    }
    TestPlugin* _test_plugin;
    StompBoxManager* _module_under_test;
};


TEST_F(StompBoxManagerTest, TestInstanciation)
{
    EXPECT_TRUE(_module_under_test->instance());
    EXPECT_EQ("test_plugin", _module_under_test->instance()->unique_id());
}


TEST_F(StompBoxManagerTest, TestParameterHandlingViaEvents)
{
    BaseStompBoxParameter* test_param = _module_under_test->register_float_parameter("param_1", "Param 1", 1, new FloatParameterPreProcessor(0.0, 10.0));

    // access the parameter through its id and verify type and that you can set its value.
    ASSERT_EQ(StompBoxParameterType::FLOAT, _module_under_test->get_parameter("param_1")->type());
    ParameterChangeEvent event(EventType::FLOAT_PARAMETER_CHANGE, "processor", 0, "param_1", 6.0f);
    _module_under_test->process_event(&event);
    EXPECT_FLOAT_EQ(6.0f, static_cast<FloatStompBoxParameter*>(test_param)->value());

    test_param = _module_under_test->register_int_parameter("param_2", "Param 2", 1, new IntParameterPreProcessor(0, 10));
    EXPECT_EQ(StompBoxParameterType::INT, test_param->type());

    test_param = _module_under_test->register_bool_parameter("param_3", "Param 3", true);
    EXPECT_EQ(StompBoxParameterType::BOOL, test_param->type());

    test_param = _module_under_test->register_string_parameter("param_4", "Param 4", "4");
    ASSERT_EQ(StompBoxParameterType::STRING, _module_under_test->get_parameter("param_4")->type());
    std::string* str_value = new std::string("5");
    StringParameterChangeEvent event_4("processor", 0, "param_4", str_value);
    _module_under_test->process_event(&event_4);
    EXPECT_EQ("5", *static_cast<StringStompBoxParameter*>(_module_under_test->get_parameter("param_4"))->value());

    //test that an unknown parameter returns a null pointer
    EXPECT_EQ(nullptr, _module_under_test->get_parameter("not_registered"));
}