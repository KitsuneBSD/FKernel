#!/usr/bin/env lua

local LogFileIO = require("Meta.Lib.log_file_io")
local LogAnalysisCore = require("Meta.Lib.log_analysis_core")
local LogReportGenerator = require("Meta.Lib.log_report_generator")
local QemuRunner = require("Meta.Lib.qemu_runner")

local LogAnalyzer = {}

function LogAnalyzer.analyze_all(filename, output_dir)
	print("\n" .. string.rep("=", 80))
	print("                    FKERNEL LOG ANALYZER")
	print(string.rep("=", 80) .. "\n")

	LogFileIO.ensure_directory(output_dir)
	print("✓ Output directory: " .. output_dir .. "\n")

	print("Reading log file...")
	local lines = LogFileIO.read_log(filename)
	if not lines then
		return false
	end

	print("\nAnalyzing log data...\n")

	-- Existing analysis
	local errors = LogAnalysisCore.find_errors(lines)
	local warnings = LogAnalysisCore.find_warnings(lines)
	local smm_data = LogAnalysisCore.count_smm_transitions(lines)
	local leaks = LogAnalysisCore.find_memory_leaks(lines)
	local subsystems = LogAnalysisCore.check_subsystem_init(lines)
	local log_types = LogAnalysisCore.count_log_types(lines)
	local int_data = LogAnalysisCore.analyze_interrupts(lines)
	local last_lines = LogAnalysisCore.get_last_lines(lines, 100)

	---------------------------------------------------------------------
	-- NEW: Timestamp extraction + delta analysis
	---------------------------------------------------------------------
	local timestamps = LogAnalysisCore.extract_timestamps(lines)
	local ts_gaps = LogAnalysisCore.compute_time_gaps(timestamps, 0.5)

	---------------------------------------------------------------------
	-- NEW: Silent period detector
	---------------------------------------------------------------------
	local silence = LogAnalysisCore.detect_silence(lines, 40)

	---------------------------------------------------------------------
	-- NEW: Interrupt storms
	---------------------------------------------------------------------
	local int_storms = LogAnalysisCore.detect_interrupt_storms(int_data, 2000)

	---------------------------------------------------------------------
	-- NEW: Incomplete subsystem initialization
	---------------------------------------------------------------------
	local init_incomplete = LogAnalysisCore.detect_incomplete_init(subsystems)

	---------------------------------------------------------------------
	-- NEW: Stack failures
	---------------------------------------------------------------------
	local stack_fail = LogAnalysisCore.find_stack_failures(lines)

	---------------------------------------------------------------------
	-- NEW: Paging/MMU failures
	---------------------------------------------------------------------
	local mmu_fail = LogAnalysisCore.find_paging_failures(lines)
	---------------------------------------------------------------------

	print("Generating reports...\n")

	LogReportGenerator.output_errors(errors, output_dir)
	LogReportGenerator.output_warnings(warnings, output_dir)
	LogReportGenerator.output_smm(smm_data, output_dir)
	LogReportGenerator.output_leaks(leaks, output_dir)
	LogReportGenerator.output_subsystems(subsystems, output_dir)
	LogReportGenerator.output_statistics(log_types, #lines, output_dir)
	LogReportGenerator.output_interrupts(int_data, output_dir)
	LogReportGenerator.output_last_lines(last_lines, output_dir)

	-- NEW report outputs (criaremos no generator)
	LogReportGenerator.output_timestamps(timestamps, ts_gaps, output_dir)
	LogReportGenerator.output_silence_periods(silence, output_dir)
	LogReportGenerator.output_interrupt_storms(int_storms, output_dir)
	LogReportGenerator.output_incomplete_init(init_incomplete, output_dir)
	LogReportGenerator.output_stack_failures(stack_fail, output_dir)
	LogReportGenerator.output_mmu_failures(mmu_fail, output_dir)

	-- Update summary
	LogReportGenerator.output_summary({
		filename = filename,
		total_lines = #lines,
		errors_count = #errors,
		warnings_count = #warnings,
		leaks_count = #leaks,
		smm_enter = smm_data.enter,
		smm_exit = smm_data.exit,
		subsystems = subsystems,
		log_types = log_types,

		-- NEW summary fields
		timestamp_gaps = #ts_gaps,
		silence_count = #silence,
		interrupt_storms = #int_storms,
		incomplete_init = #init_incomplete,
		stack_failures = #stack_fail,
		mmu_failures = #mmu_fail,
	}, output_dir)

	print("\n" .. string.rep("=", 80))
	print("Analysis complete! Reports saved to: " .. output_dir)
	print(string.rep("=", 80) .. "\n")

	print("QUICK SUMMARY:")
	print("  Total lines:          " .. #lines)
	print("  Errors:               " .. #errors)
	print("  Warnings:             " .. #warnings)
	print("  Memory leaks:         " .. #leaks)
	print("  SMM transitions:      " .. smm_data.enter .. " enter, " .. smm_data.exit .. " exit")
	print("  Timestamp gaps:       " .. #ts_gaps)
	print("  Silence blocks:       " .. #silence)
	print("  Interrupt storms:     " .. #int_storms)
	print("  Incomplete init:      " .. #init_incomplete)
	print("  Stack failures:       " .. #stack_fail)
	print("  MMU failures:         " .. #mmu_fail)
	print("\nCheck 00_SUMMARY.txt for detailed analysis.\n")

	return true
end

function Main()
	print([[
    ███████╗██╗  ██╗███████╗██████╗ ███╗   ██╗███████╗██╗     
    ██╔════╝██║ ██╔╝██╔════╝██╔══██╗████╗  ██║██╔════╝██║     
    █████╗  █████╔╝ █████╗  ██████╔╝██╔██╗ ██║█████╗  ██║     
    ██╔══╝  ██╔═██╗ ██╔══╝  ██╔══██╗██║╚██╗██║██╔══╝  ██║     
    ██║     ██║  ██╗███████╗██║  ██║██║ ╚████║███████╗███████╗
    ╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝
    
    LOG ANALYZER v2.0 - Automated Analysis
    ]])

	local debug_log_filename = arg[1] or "logs/qemu_debug.log"
	local output_dir = arg[2] or "logs/analysis"

	-- Run QEMU with specified debug flags and capture output
	local qemu_debug_flags = "int,mmu"
	local qemu_ok = QemuRunner.run_qemu_and_capture_log(qemu_debug_flags, debug_log_filename)

	if not qemu_ok then
		print("ERROR: QEMU did not run successfully or failed to capture log.")
		os.exit(1)
	end

	-- Now analyze the generated debug log
	local success = LogAnalyzer.analyze_all(debug_log_filename, output_dir)

	if not success then
		os.exit(1)
	end
end

Main()
