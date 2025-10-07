CC = gcc
CFLAGS = -Wall -O2
TARGET = game

all:
	@if [ -z "$(FILE)" ]; then \
		echo "⚠️  사용법: make FILE=파일명 [DIR=경로]"; \
		echo "예시1: make FILE=tictactoe"; \
		echo "예시2: make DIR=subfolder FILE=snake"; \
	else \
		FILEPATH="$(if $(DIR),$(DIR)/$(FILE).c,$(FILE).c)"; \
		if [ -f "$$FILEPATH" ]; then \
			echo "🛠️  컴파일 중: $$FILEPATH"; \
			$(CC) $(CFLAGS) "$$FILEPATH" -o $(TARGET); \
			echo "✅ 컴파일 완료!"; \
			echo "🎮 실행 중..."; \
			./$(TARGET); \
		else \
			echo "❌ 파일을 찾을 수 없습니다: $$FILEPATH"; \
		fi \
	fi

clean:
	rm -f $(TARGET)
