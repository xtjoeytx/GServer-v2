def notify(status){
	emailext (
		body: '$DEFAULT_CONTENT',
		recipientProviders: [
			[$class: 'CulpritsRecipientProvider'],
			[$class: 'DevelopersRecipientProvider'],
			[$class: 'RequesterRecipientProvider']
		],
		replyTo: '$DEFAULT_REPLYTO',
		subject: '$DEFAULT_SUBJECT',
		to: '$DEFAULT_RECIPIENTS'
	)
}

@NonCPS
def killall_jobs() {
	def jobname = env.JOB_NAME;
	def buildnum = env.BUILD_NUMBER.toInteger();
	def killnums = "";
	def job = Jenkins.instance.getItemByFullName(jobname);
	def fixed_job_name = env.JOB_NAME.replace('%2F','/');
	def split_job_name = env.JOB_NAME.split(/\/{1}/);

	for (build in job.builds) {
		if (!build.isBuilding()) { continue; }
		if (buildnum == build.getNumber().toInteger()) { continue; println "equals"; }
		if (buildnum < build.getNumber().toInteger()) { continue; println "newer"; }

		echo "Kill task = ${build}";

		killnums += "#" + build.getNumber().toInteger() + ", ";

		build.doStop();
	}

	if (killnums != "") {
		discordSend description: "in favor of #${buildnum}, ignore following failed builds for ${killnums}", footer: "", link: env.BUILD_URL, result: "ABORTED", title: "[${split_job_name[0]}] Killing task(s) ${fixed_job_name} ${killnums}", webhookURL: env.GS2EMU_WEBHOOK;
	}
	echo "Done killing"
}

def buildStepDocker(config) {
	def DOCKER_ROOT = config.DockerRoot;
	def DOCKERIMAGE	= config.DockerImage;
	def DOCKERTAG	= config.Tag;
	def DOCKERFILE	= config.Dockerfile;
	def BUILD_NEXT	= config.BuildIfSuccessful;
	def BUILDENV	= config.BuildEnv;
	def RUN_TESTS	= config.RunTests;
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');

	try {
		checkout scm;

		def buildenv = "${DOCKERTAG}";
		def tag = '';
		def EXTRA_VER = '';
		def PUSH_IMAGE = BUILD_NEXT.equals('image');
		def PUSH_ARTIFACT = BUILD_NEXT.equals('artifact');

		if (BUILD_NEXT.equals('both')) {
			PUSH_IMAGE = BUILD_NEXT.equals('both');
			PUSH_ARTIFACT = BUILD_NEXT.equals('both');
		}

		if(env.TAG_NAME) {
			sh(returnStdout: true, script: "echo '```' > RELEASE_DESCRIPTION.txt");
			env.RELEASE_DESCRIPTION = sh(returnStdout: true, script: "git tag -l --format='%(contents)' ${env.TAG_NAME} >> RELEASE_DESCRIPTION.txt");
			sh(returnStdout: true, script: "echo '```' >> RELEASE_DESCRIPTION.txt");
		}

		if (env.BRANCH_NAME.equals('master')) {
			tag = "latest${DOCKERTAG}";
		} else {
			tag = "${env.BRANCH_NAME.replace('/','-')}${DOCKERTAG}";
			EXTRA_VER = "--build-arg VER_EXTRA=-${tag}";
		}

		docker.withRegistry("https://index.docker.io/v1/", "dockergraal") {
			def release_name = env.JOB_NAME.replace('%2F','/');
			def release_type = ("${release_name}").replace('/','-').replace('GServer-v2-','').replace('master','').replace('dev','');

			def customImage
			stage("Building ${DOCKERIMAGE}:${tag}...") {
				customImage = docker.build("${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", "--build-arg BUILDENV=${buildenv} ${EXTRA_VER} ${BUILDENV} --network=host --pull -f ${DOCKERFILE} .");
			}

			def archive_date = sh (
				script: 'date +"-%Y%m%d-%H%M"',
				returnStdout: true
			).trim();

			if (env.TAG_NAME) {
				archive_date = '';
			}

			if (PUSH_IMAGE) {
				stage("Pushing to docker hub registry...") {
					customImage.push();
					discordSend description: "Docker Image: ${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Image Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK;
				}
			}

			if (RUN_TESTS) {
				stage("Run tests...") {
					customImage.inside("") {
						try{
							sh "cd /tmp/gserver/build && ctest -T test --no-compress-output --output-on-failure"
						} catch(err) {
							currentBuild.result = 'FAILURE'

							discordSend description: "Testing Failed: ${fixed_job_name} #${env.BUILD_NUMBER} DockerImage: ${DOCKERIMAGE} (<${env.BUILD_URL}|Open>)", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK
							notify('Build failed')
						}

						sh "mkdir -p ./test && cp -fvr /tmp/gserver/build/Testing ./test/Testing"
						dir("./test") {
							archiveArtifacts (
								artifacts: 'Testing/**/*.xml',
								fingerprint: true
							)
							stage("Xunit") {
								// Process the CTest xml output with the xUnit plugin
								xunit (
									testTimeMargin: '3000',
									thresholdMode: 1,
									thresholds: [
										skipped(failureThreshold: '0'),
										failed(failureThreshold: '0')
									],
									tools: [CTest(
										pattern: 'Testing/**/*.xml',
										deleteOutputFiles: true,
										failIfNotNew: false,
										skipNoTestFiles: true,
										stopProcessingIfError: true
									)],
									skipPublishingChecks: false
								);
							}
						}
					}
				}
			}

			if (PUSH_ARTIFACT) {
				stage("Archiving artifacts...") {
					customImage.inside("") {
						sh "mkdir -p ./dist && cp -fvr /dist/* ./dist"

						dir("./dist") {
							archiveArtifacts artifacts: '*.zip,*.tar.gz,*.tgz', allowEmptyArchive: true
							discordSend description: "Docker Image: ${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Artifact Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK;
						}
					}
					def dockerImageRef = docker.image("amigadev/docker-base:latest");
					dockerImageRef.pull();

					dockerImageRef.inside("") {

						stage("Github Release") {
							withCredentials([string(credentialsId: 'PREAGONAL_GITHUB_TOKEN', variable: 'GITHUB_TOKEN')]) {
								dir("./dist") {
									if (!env.CHANGE_ID) { // Don't run on PR's
										def release_type_tag = 'develop';
										def pre_release = '--pre-release';
										if (env.TAG_NAME) {
											pre_release = '';
											release_type_tag = env.TAG_NAME;
										} else if (env.BRANCH_NAME.equals('master')) {
											release_type_tag = 'nightly';
										}


										if (!env.TAG_NAME) {
											sh(returnStdout: true, script: "echo -e '${release_type_tag} releases' > ../RELEASE_DESCRIPTION.txt");
										}

										def files = sh(returnStdout: true, script: 'find . -name "*.zip" -o -name "*.tar.gz"');
										files = sh (script: "basename ${files}",returnStdout:true).trim()

										try {
											sh "cat ../RELEASE_DESCRIPTION.txt | github-release release --user xtjoeytx --repo GServer-v2 --tag ${release_type_tag} --name \"GS2Emu ${release_type_tag}\" ${pre_release} --description -"
										} catch(err) {

										}
										sh "github-release upload --user xtjoeytx --repo GServer-v2 --tag ${release_type_tag} --name \"${files}\" --file ${files} --replace"
									}
								}
							}
						}
					}
				}
			} else {
				// Do nothing
			}
		}
	} catch(err) {
		currentBuild.result = 'FAILURE'

		discordSend description: "", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK

		notify("Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}")
		throw err
	}
}

node('master') {
	killall_jobs();
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');
	checkout(scm);

	env.COMMIT_MSG = sh (
		script: 'git log -1 --pretty=%B ${GIT_COMMIT}',
		returnStdout: true
	).trim();

	discordSend description: "${env.COMMIT_MSG}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Started: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK

	def branches = [:]
	def project = readJSON file: "JenkinsEnv.json";
	if (env.TAG_NAME) {
		sh(returnStdout: true, script: "echo '```' > RELEASE_DESCRIPTION.txt");
		env.RELEASE_DESCRIPTION = sh(returnStdout: true, script: "git tag -l --format='%(contents)' ${env.TAG_NAME} >> RELEASE_DESCRIPTION.txt");
		sh(returnStdout: true, script: "echo '```' >> RELEASE_DESCRIPTION.txt");
	}

	project.builds.each { v ->
		branches["Build ${v.Title}"] = {
			node(v.OS) {
				if ("${v.Type}" == "docker") {
					buildStepDocker(v.Config);
				}
			}
		}
	}

	parallel branches;

	if (env.TAG_NAME) {
		def DESC = sh(returnStdout: true, script: 'cat RELEASE_DESCRIPTION.txt');
		discordSend description: "${DESC}", customUsername: "OpenGraal", customAvatarUrl: "https://pbs.twimg.com/profile_images/1895028712/13460_106738052711614_100001262603030_51047_4149060_n_400x400.jpg", footer: "OpenGraal Team", link: "https://github.com/xtjoeytx/GServer-v2/releases/tag/${env.TAG_NAME}", result: "SUCCESS", title: "GS2Emu v${env.TAG_NAME}", webhookURL: env.GS2EMU_RELEASE_WEBHOOK;
	}
	sh "rm -rf ./*"
}
