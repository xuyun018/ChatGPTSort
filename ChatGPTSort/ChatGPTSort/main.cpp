#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <pthread.h>

#define MAX_LINE_LENGTH 1000 // ����ÿ����󳤶�Ϊ1000���ַ�
#define NUM_THREADS 40 // ����ʹ��4���߳̽��в��д���
#define MAX_CHUNK_SIZE 10000000 // ����ÿ����������1000��������

typedef struct {
	char **lines;
	int lineCount;
	int i;
} ChunkData;

// �ȽϺ���������qsort����
int compare(const void *a, const void *b) {
	return strcmp(*(const char **)a, *(const char **)b);
}

// ������õ�����д���ļ�
void writeToFile(const char *filename, char **lines, int count) {
	FILE *file = fopen(filename, "w");
	if (file == NULL) {
		perror("Error opening file for writing");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < count; ++i) {
		fputs(lines[i], file);
	}
	fclose(file);
}

// �ⲿ��������
//void *externalSortThread(void *chunkData)
DWORD WINAPI externalSortThread(void *chunkData)
{
	ChunkData *data = (ChunkData *)chunkData;
	char **lines = data->lines;
	int i;

	// ʹ�ÿ��������㷨�����ݽ�������
	qsort(lines, data->lineCount, sizeof(char *), compare);

	char chunkFilename[20];
	sprintf(chunkFilename, "chunk_%d.txt", data->i);
	writeToFile(chunkFilename, data->lines, data->lineCount);

	for (i = 0; i < data->lineCount; i++)
	{
		free(lines[i]);
	}
	free(lines);

	free(data);

	return(0);
	//pthread_exit(NULL);
}

// ���ļ��ֿ鲢����
unsigned int sortChunks(const char *inputFilename, int numThreads) {
	FILE *inputFile = fopen(inputFilename, "r");
	if (inputFile == NULL) {
		perror("Error opening input file");
		exit(EXIT_FAILURE);
	}

	char buffer[MAX_LINE_LENGTH];
	int lineCount = 0;
	int chunkNum = 0;
	unsigned int j = 0;
	unsigned int l;
	//char *lines[MAX_CHUNK_SIZE];
	char **lines;

	lines = (char **)malloc(MAX_CHUNK_SIZE * sizeof(char *));

	// �����߳�����
	//pthread_t threads[numThreads];
	//int threadArgs[numThreads];
	HANDLE *hthreads = (HANDLE *)malloc(sizeof(HANDLE) * numThreads);

	// ���ж�ȡ�ļ�����
	while (fgets(buffer, MAX_LINE_LENGTH, inputFile) != NULL) {
		// �����ļ���û�п���
		l = strlen(buffer);
		lines[lineCount] = (char *)malloc(l + 1);
		if (lines[lineCount])
		{
			strcpy(lines[lineCount], buffer);
		}
		if (lines[lineCount] == NULL) {
			perror("Memory allocation error");
			exit(EXIT_FAILURE);
		}
		++lineCount;
		if (lineCount == MAX_CHUNK_SIZE) {
			// ÿ��ȡһ�����ݾ�����һ���߳̽�������
			ChunkData *data = (ChunkData *)malloc(sizeof(ChunkData));
			data->lines = lines;
			data->lineCount = lineCount;
			data->i = j++;

			hthreads[chunkNum] = CreateThread(NULL, 0, externalSortThread, data, 0, NULL);
			//threadArgs[chunkNum] = pthread_create(&threads[chunkNum], NULL, externalSortThread, (void *)data);
			//if (threadArgs[chunkNum])
			if (hthreads[chunkNum] == NULL)
			{
				int i;

				for (i = 0; i < lineCount; i++)
				{
					free(lines[i]);
				}
				free(lines);

				free(data);

				//printf("Error: Unable to create thread %d\n", threadArgs[chunkNum]);
				printf("Error: Unable to create thread %d\n", chunkNum);
				exit(EXIT_FAILURE);
			}

			// ��ջ�������׼����ȡ��һ������
			lineCount = 0;
			lines = (char **)malloc(MAX_CHUNK_SIZE * sizeof(char *));
			if (lines == NULL) {
				perror("Memory allocation error");
				exit(EXIT_FAILURE);
			}
			chunkNum++;
		}
	}
	fclose(inputFile);

	printf("%d, %d\r\n", lineCount, MAX_CHUNK_SIZE);

	if (lineCount) {
		// ÿ��ȡһ�����ݾ�����һ���߳̽�������
		ChunkData *data = (ChunkData *)malloc(sizeof(ChunkData));
		data->lines = lines;
		data->lineCount = lineCount;
		data->i = j++;

		externalSortThread(data);
	}
	else
	{
		free(lines);
	}

	// �ȴ������߳̽���
	for (int i = 0; i < chunkNum; i++) {
		WaitForSingleObject(hthreads[i], INFINITE);
		//pthread_join(threads[i], NULL);
	}

	return(chunkNum + 1);
}

// �ϲ������Ŀ�
void mergeChunks(const char *outputFilename, int numChunks) {
	// ����һ���ļ�ָ�����飬���ڴ洢ÿ������ļ�ָ��
	//FILE *chunkFiles[numChunks];

	FILE **chunkFiles = (FILE **)malloc(sizeof(FILE *)*numChunks);

	// ��ÿ�����ļ�
	for (int i = 0; i < numChunks; i++) {
		char chunkFilename[20];
		sprintf(chunkFilename, "chunk_%d.txt", i);
		chunkFiles[i] = fopen(chunkFilename, "r");
		if (chunkFiles[i] == NULL) {
			perror("Error opening chunk file");
			exit(EXIT_FAILURE);
		}
	}

	// ����һ������ļ�ָ��
	FILE *outputFile = fopen(outputFilename, "w");
	if (outputFile == NULL) {
		perror("Error opening output file");
		exit(EXIT_FAILURE);
	}

	// ����һ���������洢ÿ���鵱ǰ����
	//char *currentLines[numChunks];
	char **currentLines = (char **)malloc(sizeof(char *)*numChunks);
	for (int i = 0; i < numChunks; i++) {
		currentLines[i] = (char *)malloc(MAX_LINE_LENGTH * sizeof(char));
		if (currentLines[i] == NULL) {
			perror("Memory allocation error");
			exit(EXIT_FAILURE);
		}
		if (fgets(currentLines[i], MAX_LINE_LENGTH, chunkFiles[i]) == NULL) {
			// ������ļ�Ϊ�գ��򽫶�Ӧ��currentLines����ΪNULL
			free(currentLines[i]);
			currentLines[i] = NULL;
		}
	}

	// �ϲ������Ŀ�
	while (1) {
		int minIndex = -1;
		for (int i = 0; i < numChunks; i++) {
			if (currentLines[i] != NULL) {
				if (minIndex == -1 || strcmp(currentLines[i], currentLines[minIndex]) < 0) {
					minIndex = i;
				}
			}
		}
		if (minIndex == -1) {
			// ���п鶼�������
			break;
		}
		// ����ǰ��С����д������ļ�
		fputs(currentLines[minIndex], outputFile);
		// ��ȡ��һ������
		if (fgets(currentLines[minIndex], MAX_LINE_LENGTH, chunkFiles[minIndex]) == NULL) {
			// ����Ѿ������ļ�ĩβ����currentLines[minIndex]����ΪNULL
			free(currentLines[minIndex]);
			currentLines[minIndex] = NULL;
		}
	}

	// �ر����п��ļ�������ļ�
	for (int i = 0; i < numChunks; i++) {
		if (currentLines[i] != NULL) {
			free(currentLines[i]);
		}
		fclose(chunkFiles[i]);
	}
	fclose(outputFile);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *inputFilename = argv[1];
	const char *outputFilename = argv[2];
	unsigned int n;

	unsigned int tickcount0;
	unsigned int tickcount1;

	tickcount0 = GetTickCount();

	// ʹ�ö��̶߳��ļ��ֿ鲢��������
	n = sortChunks(inputFilename, NUM_THREADS);

	tickcount1 = GetTickCount();

	printf("Cost %d\n", tickcount1 - tickcount0);

	// ���̺߳ϲ�
	mergeChunks(outputFilename, n);
	tickcount1 = GetTickCount();

	printf("Cost %d, External sorting completed successfully!\n", tickcount1 - tickcount0);

	return EXIT_SUCCESS;
}
