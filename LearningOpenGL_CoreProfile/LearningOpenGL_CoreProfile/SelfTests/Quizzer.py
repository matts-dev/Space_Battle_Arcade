import json
import random
import os

def take_quiz(filepath):
    score = 0;
    with open(filepath) as file:
        fileContents = file.read()

    data = json.loads(fileContents)

    question_list = data["questions"]
    if question_list is None:
        print("failed to load questions")
        return
    random.shuffle(question_list)
    total = len(question_list)

    for question in question_list:
        prompt = question["question"]
        answers_list = question["answers"]
        random.shuffle(answers_list)

        print(prompt)
        for i, answer in enumerate(answers_list):
            answer_tuple = list(answer.items())[0]
            print(i, ":", answer_tuple[1])

        try:
            answer_idx = int(input("answer number? "))
            answer_tuple = list(answers_list[answer_idx].items())[0]
            if answer_tuple[0] == "*":
                print("correct")
                score += 1
            else:
                for answer in answers_list:
                    answer_tuple = list(answer.items())[0]
                    if answer_tuple[0] == "*":
                        correct_value = answer_tuple[1]
                        break
                print("incorrect")
                print("correct answer:\n", correct_value)
        except:
            print("error in response")
        print("")

    print("score: ", score, "/", total, " = ", (score/total)*100, "%")


def pick_quiz():
    files = os.listdir(".")

    should_close = False
    while not should_close:
        for idx, file in enumerate(files):
            print(idx, file)

        file_idx = int(input("Please choose a file by its index: "))
        file = files[file_idx]
        print("opening:", file, "\n\n")
        take_quiz(file)

        should_close = input("\n\ntype exit to quit:") == "exit"
        print("\n\n")


if __name__ == '__main__':
    pick_quiz()
