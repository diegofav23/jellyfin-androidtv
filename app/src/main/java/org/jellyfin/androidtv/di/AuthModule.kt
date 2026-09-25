package org.jellyfin.androidtv.di

import org.jellyfin.androidtv.auth.AccountManagerMigration
import org.jellyfin.androidtv.auth.proxy.ProxyHeadersInterceptor
import org.jellyfin.androidtv.auth.proxy.ProxyHeadersRedirectInterceptor
import org.jellyfin.androidtv.auth.proxy.ProxyHeadersRepository
import org.jellyfin.androidtv.auth.repository.AuthenticationRepository
import org.jellyfin.androidtv.auth.repository.AuthenticationRepositoryImpl
import org.jellyfin.androidtv.auth.repository.ServerRepository
import org.jellyfin.androidtv.auth.repository.ServerRepositoryImpl
import org.jellyfin.androidtv.auth.repository.ServerUserRepository
import org.jellyfin.androidtv.auth.repository.ServerUserRepositoryImpl
import org.jellyfin.androidtv.auth.repository.SessionRepository
import org.jellyfin.androidtv.auth.repository.SessionRepositoryImpl
import org.jellyfin.androidtv.auth.store.AuthenticationPreferences
import org.jellyfin.androidtv.auth.store.AuthenticationStore
import org.koin.dsl.module

val authModule = module {
	single { AccountManagerMigration(get()) }
	single { AuthenticationStore(get(), get()) }
	single { AuthenticationPreferences(get()) }
	single { ProxyHeadersRepository(get()) }
	single { ProxyHeadersInterceptor(get()) }
	single { ProxyHeadersRedirectInterceptor(get()) }

	single<AuthenticationRepository> {
		AuthenticationRepositoryImpl(get(), get(), get(), get(), get(), get(defaultDeviceInfo))
	}
	single<ServerRepository> { ServerRepositoryImpl(get(), get(), get()) }
	single<ServerUserRepository> { ServerUserRepositoryImpl(get(), get()) }
	single<SessionRepository> {
		SessionRepositoryImpl(get(), get(), get(), get(), get(defaultDeviceInfo), get(), get(), get())
	}

	factory {
		val serverRepository = get<ServerRepository>()
		serverRepository.currentServer.value?.serverVersion ?: ServerRepository.minimumServerVersion
	}
}
