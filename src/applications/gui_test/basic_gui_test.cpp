#include <gtest/gtest.h>
#include "gui_test_helper.h"
#include <filesystem>
#include <iostream>

class BasicGUITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Find the GUI executable path
        std::string gui_path = findGuiExecutable();
        ASSERT_FALSE(gui_path.empty()) << "Could not find repl_gui executable";
        
        helper = std::make_unique<GuiTestHelper>(gui_path);
        
        // Start the GUI application
        ASSERT_TRUE(helper->startGui()) << "Failed to start GUI application";
        
        // Verify debug port is working
        auto ping_response = helper->ping();
        ASSERT_EQ(ping_response["status"], "success") << "Debug port not responding to ping";
    }

    void TearDown() override {
        if (helper) {
            helper->stopGui();
        }
    }
    
private:
    std::string findGuiExecutable() {
        // Look for the executable in common locations
        std::vector<std::string> possible_paths = {
            "../repl_gui/repl_gui",
            "../../repl_gui/repl_gui", 
            "../../../build/src/applications/repl_gui/repl_gui",
            "../../../../build/src/applications/repl_gui/repl_gui"
        };
        
        for (const auto& path : possible_paths) {
            if (std::filesystem::exists(path)) {
                return std::filesystem::absolute(path);
            }
        }
        
        // Try to find it relative to the current working directory
        std::filesystem::path current_dir = std::filesystem::current_path();
        std::filesystem::path gui_path = current_dir / "src/applications/repl_gui/repl_gui";
        
        if (std::filesystem::exists(gui_path)) {
            return gui_path;
        }
        
        std::cout << "Current directory: " << current_dir << std::endl;
        std::cout << "Looked for GUI executable in:" << std::endl;
        for (const auto& path : possible_paths) {
            std::cout << "  " << path << " (exists: " << std::filesystem::exists(path) << ")" << std::endl;
        }
        std::cout << "  " << gui_path << " (exists: " << std::filesystem::exists(gui_path) << ")" << std::endl;
        
        return "";
    }

protected:
    std::unique_ptr<GuiTestHelper> helper;
};

TEST_F(BasicGUITest, ExecuteVariableAssignmentAndPrint) {
    // Clear output to start fresh
    auto clear_response = helper->clearOutput();
    ASSERT_EQ(clear_response["status"], "success");
    
    // Execute variable assignment
    auto execute_response1 = helper->executeCode("variable_a = 2");
    ASSERT_EQ(execute_response1["status"], "success") << "Failed to execute variable assignment";
    
    // Execute print statement
    auto execute_response2 = helper->executeCode("print(\"Some Text\")");
    ASSERT_EQ(execute_response2["status"], "success") << "Failed to execute print statement";
    
    // Check that the output contains expected content
    EXPECT_TRUE(helper->outputContains("Some Text")) << "Output does not contain expected print text";
    
    // Get variables and check that variable_a is there
    auto variables = helper->getVariableList();
    
    // Look for variable_a in the variables list
    bool found_variable_a = false;
    for (const auto& var : variables) {
        std::cout << "Variable: " << var << std::endl;
        if (var.find("variable_a") != std::string::npos && var.find("2") != std::string::npos) {
            found_variable_a = true;
            break;
        }
    }
    
    EXPECT_TRUE(found_variable_a) << "variable_a = 2 not found in variables list";
    
    // Get the full output for verification
    auto output_response = helper->getOutput();
    ASSERT_EQ(output_response["status"], "success");
    
    std::string full_output = output_response["output"];
    std::cout << "Full REPL output:" << std::endl;
    std::cout << full_output << std::endl;
    
    // Verify the print output is in the REPL output
    EXPECT_TRUE(full_output.find("Some Text") != std::string::npos) 
        << "Print output 'Some Text' not found in REPL output";
}

TEST_F(BasicGUITest, PingTest) {
    auto response = helper->ping();
    ASSERT_EQ(response["status"], "success");
    EXPECT_EQ(response["message"], "pong");
}

TEST_F(BasicGUITest, InputTextManipulation) {
    // Set some input text
    std::string test_input = "test_var = 42";
    auto set_response = helper->setInput(test_input);
    ASSERT_EQ(set_response["status"], "success");
    
    // Get the input text back
    auto get_response = helper->getInput();
    ASSERT_EQ(get_response["status"], "success");
    EXPECT_EQ(get_response["input"], test_input);
}

TEST_F(BasicGUITest, MultipleCommandsAndVariables) {
    // Execute a series of commands
    helper->executeCode("x = 10");
    helper->executeCode("y = 20");
    helper->executeCode("z = x + y");
    helper->executeCode("print(f'Result: {z}')");
    
    // Check variables were created
    auto variables = helper->getVariableList();
    
    std::vector<std::string> expected_vars = {"x", "y", "z"};
    for (const auto& expected : expected_vars) {
        bool found = false;
        for (const auto& var : variables) {
            if (var.find(expected) != std::string::npos) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Variable '" << expected << "' not found in variables list";
    }
    
    // Check output contains the result
    EXPECT_TRUE(helper->outputContains("Result: 30")) << "Expected calculation result not found in output";
}