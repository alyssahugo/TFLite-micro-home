import argparse

def convert_mem_big_to_little(input_file, output_file, word_size=4):
    with open(input_file, "r") as infile, open(output_file, "w") as outfile:
        for line in infile:
            word = line.strip()
            if len(word) != word_size * 2:  # Ensure correct word size in hex
                continue  # Skip invalid lines

            # Convert to little-endian by swapping byte order
            little_endian_word = "".join(reversed([word[i:i+2] for i in range(0, len(word), 2)]))
            
            outfile.write(little_endian_word + "\n")

def main():
    parser = argparse.ArgumentParser(description="Convert a big-endian .mem file to little-endian.")
    parser.add_argument("input_file", help="Path to the input .mem file")
    parser.add_argument("output_file", help="Path to save the converted .mem file")
    args = parser.parse_args()

    convert_mem_big_to_little(args.input_file, args.output_file)

if __name__ == "__main__":
    main()
