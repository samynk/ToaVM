#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>

#include <cstdlib>
#include <string>
#include <vector>
#include <optional>
#include <sstream>

#include "AccessFlags.hpp"
#include "jbc/ClassFile.hpp"

using namespace ftxui;


Element label_value(std::string label, std::string value)
{
	return hbox({
		text(label) | bold,
		text(value),
		});
}

Element render_header(const jbc::ClassFile& file)
{
	return window(
		text(" Class Header "),
		vbox({
			label_value("class:       ", file.thisClassName),
			label_value("super:       ", file.superClassName),
			label_value("version:     ",
				std::to_string(file.majorVersion) + "." +
				std::to_string(file.minorVersion)),
			label_value("access:      ", jbc::format_class_access_flags(file.accessFlags)),
			label_value("magic:       ", "0xCAFEBABE"),
			})
	);
}

void addRow(Elements& rows, int index, const std::string& kind, const std::string& value)
{
	rows.push_back(
		hbox({
			text(std::to_string(index)) | size(WIDTH, EQUAL, 5),
			text(kind) | size(WIDTH, EQUAL, 16),
			text(value) | flex,
			})
			);
}



Element render_constant_pool(const jbc::ClassFile& file)
{
	Elements rows;

	rows.push_back(
		hbox({
			text("#") | bold | size(WIDTH, EQUAL, 5),
			text("kind") | bold | size(WIDTH, EQUAL, 16),
			text("value") | bold | flex,
			})
			);

	rows.push_back(separator());

	int index = 0;
	for (const auto& entry : file.constantPool) {

		std::visit([&](const auto& entry)
			{
				using T = std::decay_t<decltype(entry)>;

				if constexpr (std::is_same_v<T, std::monostate>) {
					// skip
				}
				else if constexpr (std::is_same_v<T, jbc::CpUtf8>) {
					addRow(rows, index, "Utf8", entry.value);
				}
				else if constexpr (std::is_same_v<T, jbc::CpClass>) {
					if (auto value = file.tryGetUtf8(entry.nameIndex)) {
						addRow(rows, index, "Class", *value);
					}
				}
				else if constexpr (std::is_same_v<T, jbc::CpNameAndType>) {
					std::string nameAndType = "";
					if (auto name = file.tryGetUtf8(entry.nameIndex)) {
						nameAndType += *name;
					}
					if (auto desc = file.tryGetUtf8(entry.descriptorIndex)) {
						nameAndType += *desc;
					}
					addRow(rows, index, "Name and type", nameAndType);
				}
				else if constexpr (std::is_same_v<T, jbc::CpRef>) {
					addRow(rows, index, "Ref class", std::to_string(entry.classIndex));
				}
				else if constexpr (std::is_same_v<T, jbc::CpUnparsed>) {
					addRow(rows, index, "Unparsed", "");
				}
			}, entry);
		index++;
	}

	return window(
		text(" Constant Pool "),
		vbox(std::move(rows))

	);
}

Element render_methods(const jbc::ClassFile& file, int currentMethod)
{

	std::vector<Elements> rows;
	rows.push_back(
		{
			text("name") | bold | size(WIDTH, EQUAL, 20),
			text("descriptor") | bold | size(WIDTH, EQUAL, 18),
			text("stack") | bold | size(WIDTH, EQUAL, 8),
			text("locals") | bold | size(WIDTH, EQUAL, 8),
		}
		);
	for (const jbc::MethodInfo& me : file.methods)
	{
		rows.push_back({
				text(me.name) | size(WIDTH, EQUAL, 20),
				text(me.descriptor) | size(WIDTH, EQUAL, 18),
				text(std::to_string(me.maxStack)) | size(WIDTH, EQUAL, 8),
				text(std::to_string(me.maxLocals)) | size(WIDTH, EQUAL, 8),
			}
			);
	}
	Table methodTable{ rows };
	methodTable.SelectRow(currentMethod + 1).DecorateCells(inverted);
	return window(
		text(" Methods "),
		methodTable.Render()
	);
}

std::string opcode_name(std::uint8_t opcode)
{
	switch (opcode) {
	case 0x00: return "nop";

	case 0x02: return "iconst_m1";
	case 0x03: return "iconst_0";
	case 0x04: return "iconst_1";
	case 0x05: return "iconst_2";
	case 0x06: return "iconst_3";
	case 0x07: return "iconst_4";
	case 0x08: return "iconst_5";

	case 0x10: return "bipush";

	case 0x1a: return "iload_0";
	case 0x1b: return "iload_1";
	case 0x1c: return "iload_2";
	case 0x1d: return "iload_3";

	case 0x2a: return "aload_0";

	case 0x3b: return "istore_0";
	case 0x3c: return "istore_1";
	case 0x3d: return "istore_2";
	case 0x3e: return "istore_3";

	case 0x60: return "iadd";

	case 0x84: return "iinc";

	case 0xa2: return "if_icmpge";
	case 0xa7: return "goto";

	case 0xac: return "ireturn";
	case 0xb1: return "return";

	case 0xb4: return "getfield";
	case 0xb5: return "putfield";
	case 0xb6: return "invokevirtual";
	case 0xb7: return "invokespecial";
	case 0xb8: return "invokestatic";
	case 0xbb: return "new";
	

	default: return "<unknown>";
	}
}

std::size_t instruction_size(std::uint8_t opcode)
{
	switch (opcode) {
	case 0x10: return 2; // bipush
	case 0x84: return 3; // iinc

	case 0x99: // ifeq
	case 0x9a: // ifne
	case 0x9b: // iflt
	case 0x9c: // ifge
	case 0x9d: // ifgt
	case 0x9e: // ifle
	case 0x9f: // if_icmpeq
	case 0xa0: // if_icmpne
	case 0xa1: // if_icmplt
	case 0xa2: // if_icmpge
	case 0xa3: // if_icmpgt
	case 0xa4: // if_icmple
	case 0xa5: // if_acmpeq
	case 0xa6: // if_acmpne
	case 0xa7: // goto
	case 0xa8: // jsr
	case 0xc6: // ifnull
	case 0xc7: // ifnonnull
		return 3;
	case 0xb4: // get field
	case 0xb5: // put field
	case 0xb6: // invoke virtual
	case 0xb7: // invoke static
	case 0xb8: // invoke static
	case 0xbb: // new
		return 3;

	default:
		return 1;
	}
}

std::int16_t read_s16_be(
	std::span<const std::uint8_t> bytes,
	std::size_t index
)
{
	const auto hi = static_cast<std::uint16_t>(bytes[index]);
	const auto lo = static_cast<std::uint16_t>(bytes[index + 1]);

	const auto value = static_cast<std::uint16_t>((hi << 8) | lo);

	return static_cast<std::int16_t>(value);
}

std::uint16_t read_u16_be(
	std::span<const std::uint8_t> bytes,
	std::size_t index
)
{
	return static_cast<std::uint16_t>(
		(static_cast<std::uint16_t>(bytes[index]) << 8) |
		static_cast<std::uint16_t>(bytes[index + 1])
		);
}

std::string format_branch_target(
	std::span<const std::uint8_t> bytecode,
	std::size_t pc
)
{
	const auto offset = read_s16_be(bytecode, pc + 1);
	const auto target = static_cast<std::int32_t>(pc) + offset;

	return std::to_string(target);
}

std::string format_operands(
	std::span<const std::uint8_t> bytecode,
	std::size_t pc
)
{
	const auto opcode = bytecode[pc];

	switch (opcode) {
	case 0x10: { // bipush
		const auto value = static_cast<std::int8_t>(bytecode[pc + 1]);
		return std::to_string(value);
	}

	case 0x84: { // iinc
		const auto index = bytecode[pc + 1];
		const auto amount = static_cast<std::int8_t>(bytecode[pc + 2]);
		return std::to_string(index) + ", " + std::to_string(amount);
	}

	case 0x99: // ifeq
	case 0x9a: // ifne
	case 0x9b: // iflt
	case 0x9c: // ifge
	case 0x9d: // ifgt
	case 0x9e: // ifle

	case 0x9f: // if_icmpeq
	case 0xa0: // if_icmpne
	case 0xa1: // if_icmplt
	case 0xa2: // if_icmpge
	case 0xa3: // if_icmpgt
	case 0xa4: // if_icmple

	case 0xa5: // if_acmpeq
	case 0xa6: // if_acmpne

	case 0xa7: // goto
	case 0xa8: // jsr

	case 0xc6: // ifnull
	case 0xc7: // ifnonnull
		return format_branch_target(bytecode, pc);
	case 0xb4:
		return std::to_string(read_u16_be(bytecode, pc+1));
	default:
		return {};
	}
}

Element render_bytecode_panel(const jbc::ClassFile& file, int currentMethod)
{
	std::vector<Elements> rows;
	rows.push_back(
		{
			text("pc") | bold | size(WIDTH, EQUAL, 4),
			text("bytecode") | bold | size(WIDTH, EQUAL, 18)
		}
		);
	jbc::MethodInfo info = file.getMethodInfo(currentMethod);
	auto& bytecode = info.bytecode;
	size_t pc = 0;

	while (pc < bytecode.size())
	{
		auto op_name = opcode_name(bytecode[pc]);
		auto assembly = format_operands(bytecode, pc);

		rows.push_back(
			{
				text(std::to_string(pc)) | bold | size(WIDTH, EQUAL, 4) | color(Color::Green),
				text(op_name + " " + assembly) | bold | size(WIDTH, EQUAL, 18)
			}
		);

		pc += instruction_size(bytecode[pc]);
	}
	Table byteCodeTable{ rows };
	byteCodeTable.SelectRow(1).DecorateCells(inverted);
	return window(
		text(info.name),
		byteCodeTable.Render()
	);
}

Element render_details_panel()
{
	return window(
		text(" Selected Instruction "),
		vbox({
			label_value("pc:          ", "0004"),
			label_value("opcode:      ", "iload_2"),
			separator(),
			text("Stack effect") | bold,
			text("before: [... ]"),
			text("after:  [..., int ]"),
			separator(),
			text("Meaning") | bold,
			text("Push local variable 2 onto the operand stack."),
			})
			);
}

Element render_class_viewer(const jbc::ClassFile& file, int currentMethod)
{
	auto title =
		text(" Java Class File Viewer ") |
		bold |
		center;

	auto subtitle =
		text("version 1.0 - Koen Samyn - 2026") |
		dim |
		center;

	auto left_column =
		vbox({
			render_header(file),
			render_constant_pool(file) | flex,
			}) | flex;

	auto right_column =
		vbox({
			render_methods( file, currentMethod),
			hbox({
				render_bytecode_panel( file, currentMethod ) | flex,
				render_details_panel() | flex,
			}) | flex,
			}) | flex;

	return vbox({
		title,
		subtitle,
		separator(),
		hbox({
			left_column,
			separator(),
			right_column,
		}) | flex,
		separator(),
		text("Use up and down arrow keys to select a method.") |
			dim |
			center,
		}) | border;
}

class ClassViewerApp
{
public:
	ClassViewerApp(const jbc::ClassFile& file) :m_ClassFile{ file } {

	}
	void loop() {
		auto displayClass = Renderer([&] {
			return render_class_viewer(m_ClassFile, m_CurrentMethod);
			});
		auto screen = ScreenInteractive::Fullscreen();

		auto component = ftxui::CatchEvent(displayClass, [&](ftxui::Event event) {
			if (event == ftxui::Event::ArrowDown) {
				selectNextMethod();
				return true;
			}

			if (event == ftxui::Event::ArrowUp) {
				selectPreviousMethod();
				return true;
			}

			if (event == ftxui::Event::Escape) {
				screen.ExitLoopClosure()();
				return true;
			}

			return false;
			});

		screen.Loop(component);
	}


private:
	void selectNextMethod() {
		int numMethods = m_ClassFile.getNrOfMethods();
		m_CurrentMethod = std::min(numMethods - 1, m_CurrentMethod + 1);
	}

	void selectPreviousMethod() {
		m_CurrentMethod = std::max(0, m_CurrentMethod - 1);
	}

	const jbc::ClassFile m_ClassFile;
	int m_CurrentMethod{ 0 };
};
