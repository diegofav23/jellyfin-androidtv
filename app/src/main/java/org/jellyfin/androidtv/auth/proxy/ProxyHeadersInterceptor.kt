package org.jellyfin.androidtv.auth.proxy

import okhttp3.Interceptor
import okhttp3.Response

/**
 * Application interceptor that adds the proxy headers for the requested host. Application interceptors are also used
 * for WebSocket connections, which is not the case for network interceptors.
 */
class ProxyHeadersInterceptor(
	private val proxyHeadersRepository: ProxyHeadersRepository,
) : Interceptor {
	override fun intercept(chain: Interceptor.Chain): Response {
		val request = chain.request()
		val headers = proxyHeadersRepository.getHeaders(request.url)
		if (headers.isEmpty()) return chain.proceed(request)

		return chain.proceed(request.newBuilder().apply {
			headers.forEach { (name, value) -> header(name, value) }
		}.build())
	}
}

/**
 * Network interceptor that removes proxy headers when a redirect leads to a host that should not receive them.
 * Redirects copy the headers of the original request, so without this the secrets could leak to other hosts.
 */
class ProxyHeadersRedirectInterceptor(
	private val proxyHeadersRepository: ProxyHeadersRepository,
) : Interceptor {
	override fun intercept(chain: Interceptor.Chain): Response {
		val request = chain.request()
		val allowedHeaders = proxyHeadersRepository.getHeaders(request.url).keys.map { it.lowercase() }.toSet()
		val strippedHeaders = proxyHeadersRepository.getAllHeaderNames()
			.filter { name -> name.lowercase() !in allowedHeaders && request.header(name) != null }
		if (strippedHeaders.isEmpty()) return chain.proceed(request)

		return chain.proceed(request.newBuilder().apply {
			strippedHeaders.forEach { name -> removeHeader(name) }
		}.build())
	}
}
