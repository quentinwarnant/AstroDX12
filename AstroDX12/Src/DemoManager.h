#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class GPUPass;

struct DemoDefinition
{
	std::string name;
	std::vector<GPUPass*> passes;
	std::vector<std::string> dependencies;
	bool enabled = false;
};

class DemoManager
{
public:
	void RegisterDemo(const std::string& name,
		std::vector<GPUPass*> passes,
		std::vector<std::string> dependencies = {});

	void SetDemoEnabled(const std::string& name, bool enabled);
	bool IsDemoEnabled(const std::string& name) const;

	const std::vector<DemoDefinition>& GetDemos() const { return m_demos; }

	void LoadConfig(const std::string& configPath);
	void SaveConfig() const;

private:
	void ApplyEnabledStates();
	void EnableDependencies(const std::string& name);

	std::vector<DemoDefinition> m_demos;
	std::unordered_map<std::string, size_t> m_demoLookup;
	std::string m_configPath;
};
