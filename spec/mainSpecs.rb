describe 'database' do
        def run_script(commands)
                raw_output = nil
                IO.popen("/home/sambio/Documents/sql_clone/build-sql_clone-Desktop_Qt_5_12_0_GCC_64bit-Debug/./sql_clone", "r+") do |pipe|
                        commands.each do |command|
                        pipe.puts command
                        end

                        pipe.close_write

                        raw_output = pipe.gets(nil)
                end

                raw_output.split("\n")
        end

        it 'inserts and retrieves a row' do
                result = run_script([
                        "insert 1 user1 person1@example.com",
                        "select",
                        ".exit",
                ])

                expect(result).to match_array([
                        "sqlClone >>Executed!",
                        "sqlClone >>1, user1, person1@example.com",
                        "Executed!",
                        "sqlClone >>",
                ])
        end

        it 'prints error message when table is full' do
                script = (1..1401).map do |i|
                        "insert #{i} user#{i} person#{i}@example.com"
                end
                script << ".exit"
                result = run_script(script)
                expect(result[-2]).to eq('sqlClone >>Table is fill!')
        end

        it 'allows inserting string that are the maximum length' do
                long_username = "a" * 32
                long_email = "a" * 255
                script = [
                        "insert 1 #{long_username} #{long_email}",
                        "select",
                        ".exit",
                ]
                result = run_script(script)
                expect(result).to match_array([
                        "sqlClone >>Executed!",
                        "sqlClone >>1, #{long_username}, #{long_email}",
                        "Executed!",
                        "sqlClone >>",
                ])
        end

        it 'prints errors if strings are too long' do
                long_username = "a" * 33
                long_email = "a" * 256
                script = ([
                        "insert 1 #{long_username}, #{long_email}",
                        "select",
                        ".exit",
                ])
                result = run_script(script)
                expect(result).to match_array([
                        "sqlClone >>",
                        "sqlClone >>Executed!",
                        "sqlClone >>String is too long.",
                ])
        end

        it 'prints an error message is id is negative' do
                script = ([
                        "insert -1 sambio samuil.ivanovbg@gmail.com",
                        "select",
                        ".exit",
                ])
                result = run_script(script)
                expect(result).to match_array([
                        "sqlClone >>ID must be positive number",
                        "Executed!",
                        "sqlClone >>",
                ])
        end
end
