

void	memcpy8(void* iDst, const void* iSrc, size_t iLen);
void	memcpy32(void* iDst, const void* iSrc, size_t iLen);
void	memcpyMMX(void* iDst, const void* iSrc, size_t iLen);

// 最適なものを選んで使用
void	memcpyx(void* iDst, const void* iSrc, size_t iLen);

